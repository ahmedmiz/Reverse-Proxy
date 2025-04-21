#include "RequestHandler.h"
#include <algorithm>
#include <sstream>

HttpRequest::HttpRequest(const std::string& method, const std::string& uri, const std::string& http_version)
    : method_(method), uri_(uri), http_version_(http_version) {
    parse_uri();
}

std::string HttpRequest::method() const {
    return method_;
}

std::string HttpRequest::uri() const {
    return uri_;
}

std::string HttpRequest::path() const {
    return path_;
}

std::string HttpRequest::query_string() const {
    return query_string_;
}

std::string HttpRequest::http_version() const {
    return http_version_;
}

const std::map<std::string, std::string>& HttpRequest::headers() const {
    return headers_;
}

const std::string& HttpRequest::body() const {
    return body_;
}

std::string HttpRequest::get_header(const std::string& name, const std::string& default_value) const {
    // Convert header name to lowercase for case-insensitive comparison
    std::string lowercase_name = name;
    std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), ::tolower);
    
    auto it = headers_.find(lowercase_name);
    if (it != headers_.end()) {
        return it->second;
    }
    return default_value;
}

void HttpRequest::set_header(const std::string& name, const std::string& value) {
    // Store original header name for case preservation
    std::string lowercase_name = name;
    std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), ::tolower);
    
    headers_[lowercase_name] = value;
    header_names_[lowercase_name] = name;
}

void HttpRequest::set_body(const std::string& body) {
    body_ = body;
}

bool HttpRequest::has_header(const std::string& name) const {
    std::string lowercase_name = name;
    std::transform(lowercase_name.begin(), lowercase_name.end(), lowercase_name.begin(), ::tolower);
    
    return headers_.find(lowercase_name) != headers_.end();
}

std::map<std::string, std::string> HttpRequest::parse_query_params() const {
    std::map<std::string, std::string> params;
    
    if (query_string_.empty()) {
        return params;
    }
    
    std::istringstream stream(query_string_);
    std::string pair;
    
    while (std::getline(stream, pair, '&')) {
        size_t pos = pair.find('=');
        if (pos != std::string::npos) {
            std::string key = pair.substr(0, pos);
            std::string value = pair.substr(pos + 1);
            params[key] = value;
        } else {
            // Parameter with no value
            params[pair] = "";
        }
    }
    
    return params;
}

bool HttpRequest::is_websocket_request() const {
    return method_ == "GET" && 
           get_header("Upgrade").find("websocket") != std::string::npos &&
           get_header("Connection").find("Upgrade") != std::string::npos;
}

std::string HttpRequest::get_original_header_name(const std::string& name) const {
    auto it = header_names_.find(name);
    if (it != header_names_.end()) {
        return it->second;
    }
    return name; // Return input if not found
}

void HttpRequest::parse_uri() {
    size_t pos = uri_.find('?');
    if (pos != std::string::npos) {
        path_ = uri_.substr(0, pos);
        query_string_ = uri_.substr(pos + 1);
    } else {
        path_ = uri_;
        query_string_ = "";
    }
}

HttpRequest HttpRequest::fromRawRequest(const std::string& rawRequest) {
    std::istringstream requestStream(rawRequest);
    std::string requestLine;
    std::getline(requestStream, requestLine);
    requestLine.erase(std::remove(requestLine.begin(), requestLine.end(), '\r'), requestLine.end());

    size_t methodEnd = requestLine.find(' ');
    size_t pathEnd = requestLine.find(' ', methodEnd + 1);

    if (methodEnd == std::string::npos || pathEnd == std::string::npos) {
        throw std::runtime_error("Invalid HTTP request line");
    }

    std::string method = requestLine.substr(0, methodEnd);
    std::string uri = requestLine.substr(methodEnd + 1, pathEnd - methodEnd - 1);
    std::string httpVersion = requestLine.substr(pathEnd + 1);

    HttpRequest request(method, uri, httpVersion);

    // Parse headers
    std::string line;
    while (std::getline(requestStream, line) && line != "\r") {
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string headerName = line.substr(0, colonPos);
            std::string headerValue = line.substr(colonPos + 1);
            headerValue.erase(0, headerValue.find_first_not_of(" \t"));
            headerValue.erase(headerValue.find_last_not_of(" \t") + 1);
            request.set_header(headerName, headerValue);
        }
    }

    // Parse body
    if (request.has_header("Content-Length")) {
        size_t contentLength = std::stoi(request.get_header("Content-Length"));
        std::string body(contentLength, '\0');
        requestStream.read(&body[0], contentLength);
        request.set_body(body);
    }

    return request;
}
