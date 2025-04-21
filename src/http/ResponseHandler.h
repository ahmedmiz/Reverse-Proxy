#pragma once

#include <string>
#include <map>
#include <memory>

/**
 * HTTP Status Codes
 */
enum class HttpStatus {
    OK = 200,
    CREATED = 201,
    ACCEPTED = 202,
    NO_CONTENT = 204,
    MOVED_PERMANENTLY = 301,
    FOUND = 302,
    SEE_OTHER = 303,
    NOT_MODIFIED = 304,
    TEMPORARY_REDIRECT = 307,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    TOO_MANY_REQUESTS = 429,
    INTERNAL_SERVER_ERROR = 500,
    NOT_IMPLEMENTED = 501,
    BAD_GATEWAY = 502,
    SERVICE_UNAVAILABLE = 503,
    GATEWAY_TIMEOUT = 504
};

/**
 * HTTP Response class
 * Represents an HTTP response to send to a client
 */
class HttpResponse {
public:
    /**
     * Default constructor creates 200 OK response
     */
    HttpResponse();
    
    /**
     * Constructor with status code
     * @param status HTTP status code
     */
    explicit HttpResponse(HttpStatus status);
    
    // Getters
    HttpStatus status() const;
    const std::map<std::string, std::string>& headers() const;
    const std::string& body() const;
    
    /**
     * Get HTTP status code as integer
     */
    int status_code() const;
    
    /**
     * Get HTTP status message for the current status code
     */
    std::string status_message() const;
    
    /**
     * Set the HTTP status code
     * @param status New status code
     */
    void set_status(HttpStatus status);
    
    /**
     * Set a response header
     * @param name Header name
     * @param value Header value
     */
    void set_header(const std::string& name, const std::string& value);
    
    /**
     * Set the response body
     * @param body Response body content
     * @param content_type Optional content type (default: text/plain)
     */
    void set_body(const std::string& body, const std::string& content_type = "text/plain");
    
    /**
     * Get specific header value
     * @param name Header name (case-insensitive)
     * @param default_value Value to return if header not found
     * @return Header value or default
     */
    std::string get_header(const std::string& name, const std::string& default_value = "") const;
    
    /**
     * Convert response to a string ready to send
     * @return String representation of the HTTP response
     */
    std::string to_string() const;

private:
    HttpStatus status_;
    std::map<std::string, std::string> headers_;
    std::string body_;
    
    /**
     * Get the status message for a given status code
     * @param status HTTP status code
     * @return Status message string
     */
    std::string get_status_message(HttpStatus status) const;
};

using HttpResponsePtr = std::shared_ptr<HttpResponse>;