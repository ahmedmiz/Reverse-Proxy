#include "Config.h"
#include "../util/Logger.h"
#include <fstream>
#include <json/json.h>

Config::Config() : 
    http_port_(8080),
    websocket_port_(8081),
    websocket_enabled_(false),
    ssl_enabled_(false),
    jwt_auth_enabled_(false),
    rate_limit_(100),
    rate_window_seconds_(60),
    gzip_enabled_(true),
    redis_host_("localhost"),
    redis_port_(6379)
{
}

Config::~Config() {
}

bool Config::load(const std::string& config_path) {
    try {
        // Open the config file
        std::ifstream config_file(config_path);
        if (!config_file.is_open()) {
            // LOG_ERROR("Failed to open config file: " + config_path);
            return false;
        }
        
        // parse JSON
        Json::CharReaderBuilder reader;
        Json::Value root;
        std::string errors;
        
        if (!Json::parseFromStream(reader, config_file, &root_, &errors)) {
            // LOG_ERROR("Failed to parse JSON config: " + errors);
            return false;
        }
        
        // Read server configuration
        // will resolve to 8080 
        http_port_ = root_["server"]["http_port"].asInt();
        
        // Read websocket configuration
        websocket_enabled_ = root_["server"]["websocket_enabled"].asBool();
        if (websocket_enabled_) {
            // will resolve to 8081 
            websocket_port_ = root_["server"]["websocket_port"].asInt();
        }
        
        // Read SSL configuration
        ssl_enabled_ = root_["security"]["ssl_enabled"].asBool();
        if (ssl_enabled_) {
            ssl_cert_path_ = root_["security"]["ssl_cert_path"].asString();
            ssl_key_path_ = root_["security"]["ssl_key_path"].asString();
        }
        
        // Read JWT configuration
        jwt_auth_enabled_ = root_["security"]["jwt_auth_enabled"].asBool();
        if (jwt_auth_enabled_) {
            jwt_secret_ = root_["security"]["jwt_secret"].asString();
        }
        
        // Read rate limiting configuration
        rate_limit_ = root_["performance"]["rate_limit"].asInt();
        rate_window_seconds_ = root_["performance"]["rate_window_seconds"].asInt();
        
        // Read compression configuration
        gzip_enabled_ = root_["performance"]["gzip_enabled"].asBool();
        
        // Read Redis configuration
        redis_host_ = root_["cache"]["redis_host"].asString();
        redis_port_ = root_["cache"]["redis_port"].asInt();
        if (root_["cache"].isMember("redis_password")) {
            redis_password_ = root_["cache"]["redis_password"].asString();
        }
        
        // Read CORS configuration
        const Json::Value& origins = root_["security"]["cors"]["allowed_origins"];
        for (const auto& origin : origins) {
            allowed_origins_.push_back(origin.asString());
        }
        
        // Read IP whitelist
        const Json::Value& ips = root_["security"]["ip_whitelist"];
        for (const auto& ip : ips) {
            allowed_ips_.push_back(ip.asString());
        }
        
        // Parse routes
        parse_routes(root_["routes"]);
        
        // LOG_INFO("Configuration loaded successfully");
        return true;
    }
    catch (const std::exception& e) {
        // LOG_ERROR("Error loading configuration: " + std::string(e.what()));
        return false;
    }
}

void Config::parse_routes(const Json::Value& routes_config) {
    routes_.clear();
    
    for (const auto& route_json : routes_config) {
        std::string path_prefix = route_json["path_prefix"].asString();
        RouteConfig route(path_prefix);
        
        // Parse WebSocket flag for this route
        route.websocket_enabled = route_json["websocket_enabled"].asBool();
        
        // Parse caching options
        route.cache_enabled = route_json["cache_enabled"].asBool();
        if (route.cache_enabled) {
            route.cache_ttl_seconds = route_json["cache_ttl_seconds"].asInt();
        }
        
        // Parse backend servers
        const Json::Value& backends = route_json["backends"];
        for (const auto& backend : backends) {
            std::string name = backend["name"].asString();
            std::string host = backend["host"].asString();
            int port = backend["port"].asInt();
            int weight = backend["weight"].asInt();
            
            route.backends.emplace_back(name, host, port, weight);
        }
        
        routes_.push_back(route);
    }
    
    // LOG_INFO("Parsed " + std::to_string(routes_.size()) + " routes");
}

int Config::get_http_port() const {
    return http_port_;
}

int Config::get_websocket_port() const {
    return websocket_port_;
}

bool Config::is_websocket_enabled() const {
    return websocket_enabled_;
}

bool Config::is_ssl_enabled() const {
    return ssl_enabled_;
}

std::string Config::get_ssl_cert_path() const {
    return ssl_cert_path_;
}

std::string Config::get_ssl_key_path() const {
    return ssl_key_path_;
}

bool Config::is_jwt_auth_enabled() const {
    return jwt_auth_enabled_;
}

std::string Config::get_jwt_secret() const {
    return jwt_secret_;
}

int Config::get_rate_limit() const {
    return rate_limit_;
}

int Config::get_rate_window_seconds() const {
    return rate_window_seconds_;
}

bool Config::is_gzip_enabled() const {
    return gzip_enabled_;
}

std::string Config::get_redis_host() const {
    return redis_host_;
}

int Config::get_redis_port() const {
    return redis_port_;
}

std::string Config::get_redis_password() const {
    return redis_password_;
}

std::vector<std::string> Config::get_allowed_origins() const {
    return allowed_origins_;
}

std::vector<std::string> Config::get_allowed_ips() const {
    return allowed_ips_;
}

const RouteConfig* Config::find_route(const std::string& path) const {
    // Find the best matching route based on path prefix
    const RouteConfig* best_match = nullptr;
    size_t best_match_length = 0;
    
    for (const auto& route : routes_) {
        // Check if this route prefix matches the path
        if (path.compare(0, route.path_prefix.size(), route.path_prefix) == 0) {
            // If this is a longer match than the current best, use it
            if (route.path_prefix.size() > best_match_length) {
                best_match = &route;
                best_match_length = route.path_prefix.size();
            }
        }
    }
    
    return best_match;
}

const std::vector<RouteConfig>& Config::get_routes() const {
    return routes_;
}
