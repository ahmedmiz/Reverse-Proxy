#include "ResponseHandler.h"
#include <sstream>
#include <algorithm>

HttpResponse::HttpResponse() : status_(HttpStatus::OK) {
    // Set default headers
    headers_["Server"] = "C++ Reverse Proxy";
    headers_["Connection"] = "close";
}

HttpResponse::HttpResponse(HttpStatus status) : status_(status) {
    // Set default headers
    headers_["Server"] = "C++ Reverse Proxy";
    headers_["Connection"] = "close";
}

HttpStatus HttpResponse::status() const {
    return status_;
}

int HttpResponse::status_code() const {
    return static_cast<int>(status_);
}

std::string HttpResponse::status_message() const {
    return get_status_message(status_);
}

const std::map<std::string, std::string>& HttpResponse::headers() const {
    return headers_;
}

const std::string& HttpResponse::body() const {
    return body_;
}

void HttpResponse::set_status(HttpStatus status) {
    status_ = status;
}

void HttpResponse::set_header(const std::string& name, const std::string& value) {
    headers_[name] = value;
}

void HttpResponse::set_body(const std::string& body, const std::string& content_type) {
    body_ = body;
    
    // Set content type and length headers
    headers_["Content-Type"] = content_type;
    headers_["Content-Length"] = std::to_string(body.size());
}

std::string HttpResponse::get_header(const std::string& name, const std::string& default_value) const {
    // Case-insensitive header lookup
    std::string lower_name = name;
    std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
    
    for (const auto& header : headers_) {
        std::string header_name = header.first;
        std::transform(header_name.begin(), header_name.end(), header_name.begin(), ::tolower);
        
        if (header_name == lower_name) {
            return header.second;
        }
    }
    
    return default_value;
}

std::string HttpResponse::to_string() const {
    std::ostringstream ss;
    
    // Status line
    ss << "HTTP/1.1 " << status_code() << " " << status_message() << "\r\n";
    
    // Headers
    for (const auto& header : headers_) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    // Empty line separating headers from body
    ss << "\r\n";
    
    // Body
    ss << body_;
    
    return ss.str();
}

std::string HttpResponse::get_status_message(HttpStatus status) const {
    switch (status) {
        case HttpStatus::OK: return "OK";
        case HttpStatus::CREATED: return "Created";
        case HttpStatus::ACCEPTED: return "Accepted";
        case HttpStatus::NO_CONTENT: return "No Content";
        case HttpStatus::MOVED_PERMANENTLY: return "Moved Permanently";
        case HttpStatus::FOUND: return "Found";
        case HttpStatus::SEE_OTHER: return "See Other";
        case HttpStatus::NOT_MODIFIED: return "Not Modified";
        case HttpStatus::TEMPORARY_REDIRECT: return "Temporary Redirect";
        case HttpStatus::BAD_REQUEST: return "Bad Request";
        case HttpStatus::UNAUTHORIZED: return "Unauthorized";
        case HttpStatus::FORBIDDEN: return "Forbidden";
        case HttpStatus::NOT_FOUND: return "Not Found";
        case HttpStatus::METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case HttpStatus::TOO_MANY_REQUESTS: return "Too Many Requests";
        case HttpStatus::INTERNAL_SERVER_ERROR: return "Internal Server Error";
        case HttpStatus::NOT_IMPLEMENTED: return "Not Implemented";
        case HttpStatus::BAD_GATEWAY: return "Bad Gateway";
        case HttpStatus::SERVICE_UNAVAILABLE: return "Service Unavailable";
        case HttpStatus::GATEWAY_TIMEOUT: return "Gateway Timeout";
        default: return "Unknown Status";
    }
}
