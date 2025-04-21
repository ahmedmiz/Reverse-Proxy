#ifndef CONFIG_H
#define CONFIG_H

#include <string>
#include <vector>
#include <memory>
#include <json/json.h>

/**
 * Backend Server Configuration
 * Represents a destination server to which requests can be proxied
 */
struct BackendServer {
    std::string name;     // identifier for the backend
    std::string host;     // hostname or IP
    int port;             // port number
    int weight;           // weight for load balancing (higher = more traffic)
    bool is_healthy;      // health status
    
    // Constructor
    BackendServer(const std::string& n, const std::string& h, int p, int w = 1) 
        : name(n), host(h), port(p), weight(w), is_healthy(true) {}
};

/**
 * route Configuration
 * defines how URLs are mapped to backend servers
 */
struct RouteConfig {
    std::string path_prefix;              // URL path prefix to match
    std::vector<BackendServer> backends;  // Potential backend servers
    bool websocket_enabled;               // Whether this route supports WebSockets
    bool cache_enabled;                   // Whether to cache responses
    int cache_ttl_seconds;                // How long to cache responses
    
    // Constructor
    RouteConfig(const std::string& prefix) 
        : path_prefix(prefix), websocket_enabled(false), cache_enabled(false), cache_ttl_seconds(300) {}
};

/**
 * Configuration class
 * Manages the application configuration loaded from JSON
 */
class Config {
public:
    Config();
    ~Config();
    
    /**
     * Load configuration from a JSON file
     * @param config_path Path to the configuration file
     * @return True if loaded successfully
     */
    bool load(const std::string& config_path);
    
    // Getters for configuration values
    int get_http_port() const;
    int get_websocket_port() const;
    bool is_websocket_enabled() const;
    bool is_ssl_enabled() const;
    std::string get_ssl_cert_path() const;
    std::string get_ssl_key_path() const;
    bool is_jwt_auth_enabled() const;
    std::string get_jwt_secret() const;
    int get_rate_limit() const;
    int get_rate_window_seconds() const;
    bool is_gzip_enabled() const;
    std::string get_redis_host() const;
    int get_redis_port() const;
    std::string get_redis_password() const;
    std::vector<std::string> get_allowed_origins() const;
    std::vector<std::string> get_allowed_ips() const;
    
    /**
     * Find a route configuration matching a path
     * @param path Request path to match
     * @return Pointer to matched route or nullptr
     */
    const RouteConfig* find_route(const std::string& path) const;
    
    /**
     * Get all configured routes
     * @return Vector of route configurations
     */
    const std::vector<RouteConfig>& get_routes() const;

private:
    Json::Value root_;
    std::vector<RouteConfig> routes_;
    int http_port_;
    int websocket_port_;
    bool websocket_enabled_;
    bool ssl_enabled_;
    std::string ssl_cert_path_;
    std::string ssl_key_path_;
    bool jwt_auth_enabled_;
    std::string jwt_secret_;
    int rate_limit_;
    int rate_window_seconds_;
    bool gzip_enabled_;
    std::string redis_host_;
    int redis_port_;
    std::string redis_password_;
    std::vector<std::string> allowed_origins_;
    std::vector<std::string> allowed_ips_;
    
    /**
     * Parse routes from configuration
     * @param routes_config JSON array of route configurations
     */
    void parse_routes(const Json::Value& routes_config);
};

#endif // CONFIG_H