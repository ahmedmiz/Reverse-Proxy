#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include "../config/config.h"

/**
 * Load Balancer class
 * Manages backend server selection using various algorithms
 */
class LoadBalancer {
public:
    /**
     * Constructor
     * @param config Application configuration
     */
    explicit LoadBalancer(Config& config);
    
    /**
     * Select a backend server for a route
     * @param route The route configuration
     * @return Selected backend server or nullptr if none available
     */
    const BackendServer* select_backend(const RouteConfig& route);
    
    /**
     * Mark a backend server as healthy or unhealthy
     * @param route_prefix The route prefix
     * @param backend_name The backend server name
     * @param healthy Whether the server is healthy
     */
    void set_backend_health(const std::string& route_prefix, 
                           const std::string& backend_name, 
                           bool healthy);
    
    /**
     * Start health checking for all backends
     */
    void start_health_checks();
    
    /**
     * Stop health checking
     */
    void stop_health_checks();

private:
    Config& config_;
    std::map<std::string, std::map<std::string, bool>> health_status_;  // route -> backend -> health
    std::map<std::string, int> round_robin_counters_;  // route -> current index
    std::mutex mutex_;
    bool running_health_checks_;
    
    /**
     * Round-robin backend selection
     * @param route The route configuration
     * @return Selected backend or nullptr
     */
    const BackendServer* select_round_robin(const RouteConfig& route);
    
    /**
     * Weighted random backend selection
     * @param route The route configuration
     * @return Selected backend or nullptr
     */
    const BackendServer* select_weighted_random(const RouteConfig& route);
    
    /**
     * Get healthy backends for a route
     * @param route The route configuration
     * @return Vector of healthy backends
     */
    std::vector<const BackendServer*> get_healthy_backends(const RouteConfig& route);
    
    /**
     * Health check function
     * Runs in a separate thread to periodically check backend health
     */
    void health_check_worker();
    
    /**
     * Check health of a specific backend
     * @param backend The backend server to check
     * @return True if healthy
     */
    bool check_backend_health(const BackendServer& backend);
};
