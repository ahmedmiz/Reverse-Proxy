#include "loadBalancer.h"
#include "../util/logger.h"
#include <random>
#include <thread>
#include <chrono>
#include <curl/curl.h>

LoadBalancer::LoadBalancer(Config& config) 
    : config_(config), running_health_checks_(false) {
    
    // Initialize health status map with all backends marked as healthy
    for (const auto& route : config.get_routes()) {
        round_robin_counters_[route.path_prefix] = 0;
        
        for (const auto& backend : route.backends) {
            health_status_[route.path_prefix][backend.name] = true;
        }
    }
    
    // Start health checks
    start_health_checks();
}

const BackendServer* LoadBalancer::select_backend(const RouteConfig& route) {
    // Use weighted random selection strategy
    return select_weighted_random(route);
}

void LoadBalancer::set_backend_health(const std::string& route_prefix, 
                                     const std::string& backend_name, 
                                     bool healthy) {
    std::lock_guard<std::mutex> lock(mutex_);
    health_status_[route_prefix][backend_name] = healthy;
    
    Logger::getInstance().info(
        "Backend " + backend_name + " for route " + route_prefix + 
        " marked as " + (healthy ? "healthy" : "unhealthy"), "LoadBalancer");
}

void LoadBalancer::start_health_checks() {
    if (running_health_checks_) {
        return;
    }
    
    running_health_checks_ = true;
    
    // Start health check worker in a separate thread
    std::thread([this]() {
        this->health_check_worker();
    }).detach();
    
    Logger::getInstance().info("Health check worker started", "LoadBalancer");
}

void LoadBalancer::stop_health_checks() {
    running_health_checks_ = false;
    Logger::getInstance().info("Health check worker stopping", "LoadBalancer");
}

const BackendServer* LoadBalancer::select_round_robin(const RouteConfig& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto healthy_backends = get_healthy_backends(route);
    if (healthy_backends.empty()) {
        Logger::getInstance().error(
            "No healthy backends available for route " + route.path_prefix, "LoadBalancer");
        return nullptr;
    }
    
    // Get counter for this route
    int& counter = round_robin_counters_[route.path_prefix];
    
    // Select backend using round-robin
    const BackendServer* selected = healthy_backends[counter % healthy_backends.size()];
    
    // Increment counter
    counter = (counter + 1) % healthy_backends.size();
    
    return selected;
}

const BackendServer* LoadBalancer::select_weighted_random(const RouteConfig& route) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto healthy_backends = get_healthy_backends(route);
    if (healthy_backends.empty()) {
        Logger::getInstance().error(
            "No healthy backends available for route " + route.path_prefix, "LoadBalancer");
        return nullptr;
    }
    
    // Calculate total weight
    int total_weight = 0;
    for (const auto* backend : healthy_backends) {
        total_weight += backend->weight;
    }
    
    // Generate random number
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(1, total_weight);
    int random_weight = dist(gen);
    
    // Select backend based on weight
    int weight_sum = 0;
    for (const auto* backend : healthy_backends) {
        weight_sum += backend->weight;
        if (random_weight <= weight_sum) {
            return backend;
        }
    }
    
    // Should never get here, but just in case return the first backend
    return healthy_backends.front();
}

std::vector<const BackendServer*> LoadBalancer::get_healthy_backends(const RouteConfig& route) {
    std::vector<const BackendServer*> healthy_backends;
    
    for (const auto& backend : route.backends) {
        if (health_status_[route.path_prefix][backend.name]) {
            healthy_backends.push_back(&backend);
        }
    }
    
    return healthy_backends;
}

void LoadBalancer::health_check_worker() {
    while (running_health_checks_) {
        Logger::getInstance().debug("Running health checks", "LoadBalancer");
        
        // Check all backends for all routes
        for (const auto& route : config_.get_routes()) {
            for (const auto& backend : route.backends) {
                bool is_healthy = check_backend_health(backend);
                
                // Update health status if changed
                std::lock_guard<std::mutex> lock(mutex_);
                bool was_healthy = health_status_[route.path_prefix][backend.name];
                
                if (is_healthy != was_healthy) {
                    Logger::getInstance().info(
                        "Backend " + backend.name + " for route " + route.path_prefix + 
                        " changed state from " + (was_healthy ? "healthy" : "unhealthy") + 
                        " to " + (is_healthy ? "healthy" : "unhealthy"), "LoadBalancer");
                }
                
                health_status_[route.path_prefix][backend.name] = is_healthy;
            }
        }
        
        // Sleep before next check
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
}

bool LoadBalancer::check_backend_health(const BackendServer& backend) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::getInstance().error("Failed to initialize CURL for health check", "LoadBalancer");
        return false;
    }
    
    // Build health check URL
    std::string url = "http://" + backend.host + ":" + std::to_string(backend.port) + "/health";
    
    // Set options
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);  // HEAD request
    
    // Disable error output
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 0L);
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 0L);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Check if we got a response
    bool is_healthy = (res == CURLE_OK);
    
    // If successful, check HTTP status code
    if (is_healthy) {
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        is_healthy = (http_code >= 200 && http_code < 500);
    }
    
    // Clean up
    curl_easy_cleanup(curl);
    
    return is_healthy;
}
