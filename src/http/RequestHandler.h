#pragma once

#include <string>
#include <map>
#include <boost/asio.hpp>
#include <memory>

/**
 * HTTP Request class
 * Represents an HTTP request received from a client
 */
class HttpRequest {
public:
    /**
     * Constructor with raw request data
     * @param method HTTP method (GET, POST, etc.)
     * @param uri Request URI
     * @param http_version HTTP version string
     */
    HttpRequest(const std::string& method, const std::string& uri, const std::string& http_version);
    
    // Getters
    std::string method() const;
    std::string uri() const;
    std::string path() const; // URI without query string
    std::string query_string() const;
    std::string http_version() const;
    const std::map<std::string, std::string>& headers() const;
    const std::string& body() const;
    
    /**
     * Get specific header value
     * @param name Header name (case-insensitive)
     * @param default_value Value to return if header not found
     * @return Header value or default
     */
    std::string get_header(const std::string& name, const std::string& default_value = "") const;
    
    /**
     * Add or update a header
     * @param name Header name
     * @param value Header value
     */
    void set_header(const std::string& name, const std::string& value);
    
    /**
     * Set the request body
     * @param body Request body content
     */
    void set_body(const std::string& body);
    
    /**
     * Check if request has a particular header
     * @param name Header name (case-insensitive)
     * @return True if header exists
     */
    bool has_header(const std::string& name) const;
    
    /**
     * Parse query parameters from the query string
     * @return Map of parameter name to value
     */
    std::map<std::string, std::string> parse_query_params() const;
    
    /**
     * Check if the request is a WebSocket upgrade request
     * @return True if it's a WebSocket upgrade
     */
    bool is_websocket_request() const;
    
    /**
     * Convert normalized header name to original case
     * @param name Lowercase header name
     * @return Original case header name
     */
    std::string get_original_header_name(const std::string& name) const;

    static HttpRequest fromRawRequest(const std::string& rawRequest);

private:
    std::string method_;
    std::string uri_;
    std::string path_;
    std::string query_string_;
    std::string http_version_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    std::map<std::string, std::string> header_names_; // Maps lowercase names to original case
    
    /**
     * Parse the URI into path and query string
     */
    void parse_uri();
};

using HttpRequestPtr = std::shared_ptr<HttpRequest>;