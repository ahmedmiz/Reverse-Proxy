#pragma once

#include <memory>
#include <string>
#include <boost/asio.hpp>
#include "../http/RequestHandler.h"
#include "../http/ResponseHandler.h"
#include "../config/config.h"
#include "../security/auth.h"
#include "../cache/redis.h"
#include "loadBalancer.h"

/**
 * Proxy Handler class
 * Handles the core proxy functionality, including routing requests to backends
 */
class ProxyHandler : public std::enable_shared_from_this<ProxyHandler> {
public:
    /**
     * Constructor
     * @param config Application configuration
     * @param io_context Boost asio io_context
     */
    ProxyHandler(Config& config, boost::asio::io_context& io_context);
    
    /**
     * Handle an HTTP request
     * @param request The request to handle
     * @param client_ip The IP address of the client
     * @return The response to send back
     */
    HttpResponsePtr handle_request(HttpRequestPtr request, const std::string& client_ip);
    
    /**
     * Handle a WebSocket connection
     * @param request The initial HTTP request
     * @param client_socket The client socket
     * @param client_ip The IP address of the client
     * @return True if handled successfully
     */
    bool handle_websocket(HttpRequestPtr request, 
                         std::shared_ptr<boost::asio::ip::tcp::socket> client_socket,
                         const std::string& client_ip);

private:
    Config& config_;
    boost::asio::io_context& io_context_;
    std::unique_ptr<Authentication> auth_;
    std::unique_ptr<RedisClient> redis_client_;
    std::unique_ptr<LoadBalancer> load_balancer_;
    
    /**
     * Forward a request to a backend server
     * @param request The request to forward
     * @param route The matched route
     * @return The response from the backend
     */
    HttpResponsePtr forward_request(HttpRequestPtr request, const RouteConfig* route);
    
    /**
     * Apply security checks to a request
     * @param request The request to check
     * @param client_ip The IP address of the client
     * @return True if the request passes security checks
     */
    bool apply_security_checks(HttpRequestPtr request, const std::string& client_ip);
    
    /**
     * Apply CORS headers to a response
     * @param request The original request
     * @param response The response to modify
     */
    void apply_cors_headers(HttpRequestPtr request, HttpResponsePtr response);
    
    /**
     * Check if a request exceeds rate limits
     * @param client_ip The IP address of the client
     * @return True if the request is allowed
     */
    bool check_rate_limit(const std::string& client_ip);
    
    /**
     * Try to get a cached response
     * @param request The request
     * @param route The matched route
     * @return Cached response or nullptr if not found
     */
    HttpResponsePtr get_cached_response(HttpRequestPtr request, const RouteConfig* route);
    
    /**
     * Store a response in the cache
     * @param request The original request
     * @param response The response to cache
     * @param route The matched route
     */
    void cache_response(HttpRequestPtr request, HttpResponsePtr response, const RouteConfig* route);
    
    /**
     * Apply compression to a response if appropriate
     * @param request The original request
     * @param response The response to compress
     */
    void apply_compression(HttpRequestPtr request, HttpResponsePtr response);
    
    /**
     * Generate a key for caching a request
     * @param request The request
     * @return Cache key string
     */
    std::string generate_cache_key(HttpRequestPtr request);
};
