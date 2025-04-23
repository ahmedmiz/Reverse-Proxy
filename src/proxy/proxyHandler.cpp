#include "proxyHandler.h"
#include "../util/logger.h"
#include <curl/curl.h>
#include <iostream>
#include <sstream>
#include <zlib.h>

// Callback for writing CURL response data
size_t write_callback(char* ptr, size_t size, size_t nmemb, std::string* data) {
    if (data) {
        data->append(ptr, size * nmemb);
        return size * nmemb;
    }
    return 0;
}

// Callback for reading CURL request data
size_t read_callback(char* buffer, size_t size, size_t nitems, std::string* userdata) {
    size_t buffer_size = size * nitems;
    if (userdata->empty())
        return 0;

    size_t to_copy = std::min(buffer_size, userdata->size());
    memcpy(buffer, userdata->data(), to_copy);
    userdata->erase(0, to_copy);
    return to_copy;
}

// Callback for handling CURL headers
size_t header_callback(char* buffer, size_t size, size_t nitems, std::map<std::string, std::string>* headers) {
    size_t total_size = size * nitems;
    std::string header(buffer, total_size);
    
    // Remove trailing \r\n
    if (header.length() >= 2 && header.substr(header.length() - 2) == "\r\n") {
        header = header.substr(0, header.length() - 2);
    }
    
    // Skip the HTTP status line and empty lines
    if (header.empty() || header.substr(0, 4) == "HTTP") {
        return total_size;
    }
    
    // Split at colon
    size_t colon_pos = header.find(':');
    if (colon_pos != std::string::npos) {
        std::string name = header.substr(0, colon_pos);
        
        // Skip whitespace after colon
        size_t value_start = header.find_first_not_of(" \t", colon_pos + 1);
        std::string value;
        
        if (value_start != std::string::npos) {
            value = header.substr(value_start);
        }
        
        (*headers)[name] = value;
    }
    
    return total_size;
}

ProxyHandler::ProxyHandler(Config& config, boost::asio::io_context& io_context)
    : config_(config), io_context_(io_context) {
    
    // Initialize authentication
    if (config.is_jwt_auth_enabled()) {
        auth_ = std::make_unique<Authentication>(config);
        Logger::getInstance().info("JWT authentication enabled","proxyHandler.cpp");
    }
    
    // Initialize Redis client if needed
    if (!config.get_redis_host().empty()) {
        redis_client_ = std::make_unique<RedisClient>(
            config.get_redis_host(), 
            config.get_redis_port(),
            config.get_redis_password()
        );
        Logger::getInstance().info("Redis client initialized","proxyHandler.cpp");
    }
    
    // Initialize load balancer
    load_balancer_ = std::make_unique<LoadBalancer>(config);
    Logger::getInstance().info("Load balancer initialized","proxyHandler.cpp");
    
    // Initialize CURL
    curl_global_init(CURL_GLOBAL_ALL);
    Logger::getInstance().info("CURL initialized","proxyHandler.cpp");
}

HttpResponsePtr ProxyHandler::handle_request(HttpRequestPtr request, const std::string& client_ip) {
    Logger::getInstance().debug("Request from " + client_ip + ": " + request->method() + " " + request->uri());
    
    // Apply security checks
    if (!apply_security_checks(request, client_ip)) {
        Logger::getInstance().warning("Request from " + client_ip + " failed security checks");
        auto response = std::make_shared<HttpResponse>(HttpStatus::FORBIDDEN);
        response->set_body("Forbidden", "text/plain");
        apply_cors_headers(request, response);
        return response;
    }
    
    // Check rate limit
    if (!check_rate_limit(client_ip)) {
        Logger::getInstance().warning("Rate limit exceeded for client " + client_ip);
        auto response = std::make_shared<HttpResponse>(HttpStatus::TOO_MANY_REQUESTS);
        response->set_body("Rate limit exceeded", "text/plain");
        apply_cors_headers(request, response);
        return response;
    }
    
    // Find a matching route
    const RouteConfig* route = config_.find_route(request->path());
    if (!route) {
        Logger::getInstance().warning("No route found for path " + request->path());
        auto response = std::make_shared<HttpResponse>(HttpStatus::NOT_FOUND);
        response->set_body("Not Found", "text/plain");
        apply_cors_headers(request, response);
        return response;
    }
    
    // Try to get a cached response
    if (redis_client_ && route->cache_enabled) {
        auto cached_response = get_cached_response(request, route);
        if (cached_response) {
            Logger::getInstance().debug("Cache hit for " + request->uri());
            apply_cors_headers(request, cached_response);
            return cached_response;
        }
    }
    
    // Forward the request to a backend server
    Logger::getInstance().debug("Forwarding request to backend");
    auto response = forward_request(request, route);
    
    // Cache the response if appropriate
    if (redis_client_ && 
        route->cache_enabled && 
        response->status() == HttpStatus::OK && 
        request->method() == "GET") {
        cache_response(request, response, route);
    }
    
    // Apply compression if enabled
    if (config_.is_gzip_enabled()) {
        apply_compression(request, response);
    }
    
    // Apply CORS headers
    apply_cors_headers(request, response);
    
    Logger::getInstance().debug("Request handled successfully");
    return response;
}

bool ProxyHandler::handle_websocket(HttpRequestPtr request, 
                                   std::shared_ptr<boost::asio::ip::tcp::socket> client_socket,
                                   const std::string& client_ip) {
    // Find a matching route
    const RouteConfig* route = config_.find_route(request->path());
    if (!route || !route->websocket_enabled) {
        Logger::getInstance().warning("No WebSocket route found for path " + request->path());
        return false;
    }
    
    // Apply security checks
    if (!apply_security_checks(request, client_ip)) {
        Logger::getInstance().warning("WebSocket request from " + client_ip + " failed security checks");
        return false;
    }
    
    // Get backend server
    const BackendServer* backend = load_balancer_->select_backend(*route);
    if (!backend) {
        Logger::getInstance().error("No backend available for WebSocket forwarding");
        return false;
    }
    
    Logger::getInstance().info("Forwarding WebSocket connection to " + backend->host + ":" + std::to_string(backend->port));
    
    // In a real implementation, we would establish a WebSocket connection to the backend
    // and proxy data between client and backend
    // This would involve WebSocket++ or similar library to handle the protocol
    
    // Placeholder implementation
    Logger::getInstance().info("WebSocket handling is a placeholder in this implementation");
    return false;
}

HttpResponsePtr ProxyHandler::forward_request(HttpRequestPtr request, const RouteConfig* route) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        Logger::getInstance().error("Failed to initialize CURL");
        auto response = std::make_shared<HttpResponse>(HttpStatus::INTERNAL_SERVER_ERROR);
        response->set_body("Internal Server Error", "text/plain");
        return response;
    }
    
    // Select a backend server
    const BackendServer* backend = load_balancer_->select_backend(*route);
    if (!backend) {
        Logger::getInstance().error("No backend available for request forwarding");
        auto response = std::make_shared<HttpResponse>(HttpStatus::SERVICE_UNAVAILABLE);
        response->set_body("No backend available", "text/plain");
        return response;
    }
    
    // Build the backend URL
    std::string backend_url = "http://" + backend->host + ":" + std::to_string(backend->port);
    
    // Add the path and query string
    backend_url += request->path();
    if (!request->query_string().empty()) {
        backend_url += "?" + request->query_string();
    }
    
    Logger::getInstance().debug("Forwarding to: " + backend_url);
    
    // Set URL
    curl_easy_setopt(curl, CURLOPT_URL, backend_url.c_str());
    
    // Set HTTP method
    if (request->method() == "GET") {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    } else if (request->method() == "POST") {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
    } else if (request->method() == "PUT") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    } else if (request->method() == "DELETE") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else if (request->method() == "HEAD") {
        curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    } else if (request->method() == "OPTIONS") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "OPTIONS");
    } else if (request->method() == "PATCH") {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PATCH");
    } else {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, request->method().c_str());
    }
    
    // Set up headers
    struct curl_slist* curl_headers = nullptr;
    for (const auto& header : request->headers()) {
        std::string orig_name = request->get_original_header_name(header.first);
        std::string header_line = orig_name + ": " + header.second;
        curl_headers = curl_slist_append(curl_headers, header_line.c_str());
    }
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_headers);
    
    // Set request body if present
    std::string body_copy = request->body();
    if (!body_copy.empty()) {
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
        curl_easy_setopt(curl, CURLOPT_READDATA, &body_copy);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, body_copy.size());
    }
    
    // Set up response data
    std::string response_body;
    std::map<std::string, std::string> response_headers;
    
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, &response_headers);
    
    // Follow redirects
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    // Set timeout
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    // Perform the request
    CURLcode res = curl_easy_perform(curl);
    
    // Create response
    auto response = std::make_shared<HttpResponse>();
    
    if (res != CURLE_OK) {
        Logger::getInstance().error("CURL error: " + std::string(curl_easy_strerror(res)));
        response->set_status(HttpStatus::BAD_GATEWAY);
        response->set_body("Error forwarding request: " + std::string(curl_easy_strerror(res)), "text/plain");
    } else {
        // Get HTTP status code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        response->set_status(static_cast<HttpStatus>(http_code));
        
        // Set response body
        std::string content_type = "text/plain";
        if (response_headers.find("Content-Type") != response_headers.end()) {
            content_type = response_headers["Content-Type"];
        }
        response->set_body(response_body, content_type);
        
        // Copy headers from backend response to client response
        for (const auto& header : response_headers) {
            // Skip certain headers that we'll set ourselves
            if (header.first != "Content-Length" && header.first != "Connection") {
                response->set_header(header.first, header.second);
            }
        }
    }
    
    // Clean up
    curl_slist_free_all(curl_headers);
    curl_easy_cleanup(curl);
    
    return response;
}

bool ProxyHandler::apply_security_checks(HttpRequestPtr request, const std::string& client_ip) {
    // Check IP whitelist if configured
    const auto& allowed_ips = config_.get_allowed_ips();
    if (!allowed_ips.empty()) {
        bool ip_allowed = false;
        for (const auto& allowed_ip : allowed_ips) {
            if (allowed_ip == client_ip || allowed_ip == "0.0.0.0" || allowed_ip == "*") {
                ip_allowed = true;
                break;
            }
        }
        
        if (!ip_allowed) {
            Logger::getInstance().warning("Request from non-whitelisted IP: " + client_ip);
            return false;
        }
    }
    
    // Check JWT authentication if enabled
    if (auth_ && config_.is_jwt_auth_enabled()) {
        // Skip auth for OPTIONS requests (pre-flight CORS requests)
        if (request->method() == "OPTIONS") {
            return true;
        }
        
        // Get Authorization header
        std::string auth_header = request->get_header("Authorization");
        if (auth_header.empty()) {
            Logger::getInstance().warning("No Authorization header found");
            return false;
        }
        
        // Check for "Bearer " prefix
        if (auth_header.substr(0, 7) != "Bearer ") {
            Logger::getInstance().warning("Invalid Authorization header format");
            return false;
        }
        
        // Extract token
        std::string token = auth_header.substr(7);
        
        // Verify token
        if (!auth_->verify_jwt(token)) {
            Logger::getInstance().warning("JWT verification failed");
            return false;
        }
    }
    
    return true;
}

void ProxyHandler::apply_cors_headers(HttpRequestPtr request, HttpResponsePtr response) {
    // Get Origin header from request
    std::string origin = request->get_header("Origin");
    if (origin.empty()) {
        return;  // Not a CORS request
    }
    
    const auto& allowed_origins = config_.get_allowed_origins();
    bool origin_allowed = false;
    
    // Check if this origin is allowed
    for (const auto& allowed_origin : allowed_origins) {
        if (allowed_origin == "*" || allowed_origin == origin) {
            origin_allowed = true;
            break;
        }
    }
    
    if (origin_allowed) {
        // Set CORS headers
        response->set_header("Access-Control-Allow-Origin", origin);
        response->set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
        response->set_header("Access-Control-Allow-Headers", 
                            "Origin, Content-Type, Accept, Authorization, X-Requested-With");
        response->set_header("Access-Control-Allow-Credentials", "true");
        response->set_header("Access-Control-Max-Age", "3600");
    }
}

bool ProxyHandler::check_rate_limit(const std::string& client_ip) {
    // Skip rate limiting if Redis is not available
    if (!redis_client_) {
        return true;
    }
    
    int rate_limit = config_.get_rate_limit();
    int rate_window = config_.get_rate_window_seconds();
    
    // Skip rate limiting if not configured
    if (rate_limit <= 0 || rate_window <= 0) {
        return true;
    }
    
    std::string key = "rate_limit:" + client_ip;
    
    // Get current count
    int count = redis_client_->get_int(key);
    
    // If first request in window, set initial count and expiry
    if (count == 0) {
        redis_client_->set_with_expiry(key, "1", rate_window);
        return true;
    }
    
    // If under limit, increment and allow
    if (count < rate_limit) {
        redis_client_->increment(key);
        return true;
    }
    
    // Exceeded limit
    return false;
}

HttpResponsePtr ProxyHandler::get_cached_response(HttpRequestPtr request, const RouteConfig* route) {
    // Only cache GET requests
    if (request->method() != "GET") {
        return nullptr;
    }
    
    // Generate cache key
    std::string cache_key = generate_cache_key(request);
    
    // Try to get from cache
    std::string cached_data = redis_client_->get(cache_key);
    if (cached_data.empty()) {
        return nullptr;
    }
    
    // Parse cached data
    size_t header_end = cached_data.find("\r\n\r\n");
    if (header_end == std::string::npos) {
        Logger::getInstance().error("Invalid cached response format");
        return nullptr;
    }
    
    std::string headers_str = cached_data.substr(0, header_end);
    std::string body = cached_data.substr(header_end + 4);
    
    // Create response object
    auto response = std::make_shared<HttpResponse>();
    
    // Parse status code from first line
    size_t first_line_end = headers_str.find("\r\n");
    if (first_line_end != std::string::npos) {
        std::string status_line = headers_str.substr(0, first_line_end);
        size_t code_start = status_line.find(" ");
        size_t code_end = status_line.find(" ", code_start + 1);
        if (code_start != std::string::npos && code_end != std::string::npos) {
            std::string code_str = status_line.substr(code_start + 1, code_end - code_start - 1);
            int code = std::stoi(code_str);
            response->set_status(static_cast<HttpStatus>(code));
        }
    }
    
    // Parse headers
    std::istringstream headers_stream(headers_str);
    std::string line;
    
    // Skip status line
    std::getline(headers_stream, line);
    
    // Parse header lines
    while (std::getline(headers_stream, line) && !line.empty()) {
        // Remove CR from end of line if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            
            // Skip whitespace after colon
            size_t value_start = line.find_first_not_of(" \t", colon_pos + 1);
            std::string value;
            
            if (value_start != std::string::npos) {
                value = line.substr(value_start);
            }
            
            response->set_header(name, value);
        }
    }
    
    // Set body
    std::string content_type = response->get_header("Content-Type", "text/plain");
    response->set_body(body, content_type);
    
    // Add cache indicator header
    response->set_header("X-Proxy-Cache", "HIT");
    
    return response;
}

void ProxyHandler::cache_response(HttpRequestPtr request, HttpResponsePtr response, const RouteConfig* route) {
    // Only cache successful GET responses
    if (request->method() != "GET" || response->status() != HttpStatus::OK) {
        return;
    }
    
    // Skip caching if response says not to cache
    std::string cache_control = response->get_header("Cache-Control");
    if (cache_control.find("no-store") != std::string::npos ||
        cache_control.find("no-cache") != std::string::npos ||
        cache_control.find("private") != std::string::npos) {
        return;
    }
    
    // Generate cache key
    std::string cache_key = generate_cache_key(request);
    
    // Serialize response
    std::string serialized = response->to_string();
    
    // Store in Redis with TTL
    redis_client_->set_with_expiry(cache_key, serialized, route->cache_ttl_seconds);
    
    Logger::getInstance().debug("Cached response for " + request->uri() + " with TTL " + 
             std::to_string(route->cache_ttl_seconds) + "s");
}

std::string ProxyHandler::generate_cache_key(HttpRequestPtr request) {
    return "cache:" + request->method() + ":" + request->uri();
}

void ProxyHandler::apply_compression(HttpRequestPtr request, HttpResponsePtr response) {
    // Check if client accepts gzip
    std::string accept_encoding = request->get_header("Accept-Encoding");
    if (accept_encoding.find("gzip") == std::string::npos) {
        return;  // Client doesn't support gzip
    }
    
    // Get content type
    std::string content_type = response->get_header("Content-Type");
    
    // Only compress text-based content
    bool should_compress = 
        content_type.find("text/") != std::string::npos ||
        content_type.find("application/json") != std::string::npos ||
        content_type.find("application/javascript") != std::string::npos ||
        content_type.find("application/xml") != std::string::npos ||
        content_type.find("application/xhtml+xml") != std::string::npos;
    
    if (!should_compress || response->body().size() < 1024) {
        return;  // Don't compress small responses or binary data
    }
    
    // Compress the body with zlib
    const std::string& original_body = response->body();
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 | 16, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        Logger::getInstance().error("Failed to initialize zlib");
        return;
    }
    
    zs.next_in = reinterpret_cast<Bytef*>(const_cast<char*>(original_body.data()));
    zs.avail_in = original_body.size();
    
    int ret;
    char outbuffer[32768];
    std::string compressed_body;
    
    // Compress data
    do {
        zs.next_out = reinterpret_cast<Bytef*>(outbuffer);
        zs.avail_out = sizeof(outbuffer);
        
        ret = deflate(&zs, Z_FINISH);
        
        if (compressed_body.size() < zs.total_out) {
            compressed_body.append(outbuffer, zs.total_out - compressed_body.size());
        }
    } while (ret == Z_OK);
    
    deflateEnd(&zs);
    
    // If compression succeeded and resulted in smaller data
    if (ret == Z_STREAM_END && compressed_body.size() < original_body.size()) {
        // Update body and headers
        response->set_body(compressed_body, content_type);
        response->set_header("Content-Encoding", "gzip");
        
        Logger::getInstance().debug("Compressed response from " + std::to_string(original_body.size()) + 
                 " to " + std::to_string(compressed_body.size()) + " bytes");
    }
}
