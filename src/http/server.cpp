#include "server.h"
#include "../util/logger.h"
#include <iostream>
#include <boost/bind.hpp>
#include <thread>
#include <sstream>

HttpServer::HttpServer(boost::asio::io_context& io_context, int port, 
                       std::shared_ptr<ProxyHandler> proxy_handler)
    : io_context_(io_context),
      acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      proxy_handler_(proxy_handler),
      running_(false) {
}

void HttpServer::start() {
    if (running_) {
        return;
    }
    
    running_ = true;
    start_accept();
    Logger::getInstance().info("HTTP server started and listening on port " + 
                               std::to_string(acceptor_.local_endpoint().port()), "HttpServer");
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }
    
    running_ = false;
    
    // Close the acceptor
    boost::system::error_code ec;
    acceptor_.close(ec);
    
    if (ec) {
        Logger::getInstance().error("Error closing acceptor: " + ec.message(), "HttpServer");
    }
    
    Logger::getInstance().info("HTTP server stopped", "HttpServer");
}

void HttpServer::start_accept() {
    if (!running_) {
        return;
    }
    
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(io_context_);
    
    acceptor_.async_accept(*socket,
        [this, socket](const boost::system::error_code& error) {
            handle_accept(socket, error);
        });
}

void HttpServer::handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                              const boost::system::error_code& error) {
    if (error) {
        Logger::getInstance().error("Error accepting connection: " + error.message(), "HttpServer");
    } else {
        // Handle the connection in a separate thread
        std::thread([this, socket]() {
            try {
                handle_connection(socket);
            } catch (const std::exception& e) {
                Logger::getInstance().error("Exception handling connection: " + std::string(e.what()), "HttpServer");
            }
        }).detach();
    }
    
    // Continue accepting connections
    start_accept();
}

void HttpServer::handle_connection(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    try {
        boost::system::error_code error;
        
        // Get client info for logging
        std::string client_ip = socket->remote_endpoint().address().to_string();
        Logger::getInstance().debug("New connection from " + client_ip, "HttpServer");
        
        // Buffer for incoming data
        char buffer[8192];
        
        // Read data from socket
        std::string data;
        size_t bytes_read;
        
        // Read the initial headers
        do {
            bytes_read = socket->read_some(boost::asio::buffer(buffer), error);
            
            if (error && error != boost::asio::error::would_block) {
                throw boost::system::system_error(error);
            }
            
            data.append(buffer, bytes_read);
            
            // Check if we've read the end of headers
            if (data.find("\r\n\r\n") != std::string::npos) {
                break;
            }
        } while (bytes_read > 0);
        
        // Parse the request
        HttpRequestPtr request = parse_request(data);
        if (!request) {
            Logger::getInstance().error("Failed to parse HTTP request", "HttpServer");
            socket->close();
            return;
        }
        
        // Check if we need to read the body
        if (request->method() == "POST" || request->method() == "PUT" || request->method() == "PATCH") {
            // Get Content-Length header
            std::string content_length_str = request->get_header("Content-Length");
            if (!content_length_str.empty()) {
                size_t content_length = std::stoul(content_length_str);
                size_t headers_end = data.find("\r\n\r\n") + 4;
                size_t body_received = data.size() - headers_end;
                
                // If we haven't received the full body, read the rest
                if (body_received < content_length) {
                    size_t remaining = content_length - body_received;
                    char body_buffer[4096];
                    
                    while (remaining > 0) {
                        size_t to_read = std::min(remaining, sizeof(body_buffer));
                        size_t bytes = socket->read_some(boost::asio::buffer(body_buffer, to_read), error);
                        
                        if (error && error != boost::asio::error::would_block) {
                            throw boost::system::system_error(error);
                        }
                        
                        data.append(body_buffer, bytes);
                        remaining -= bytes;
                    }
                }
                
                // Extract body and set in request
                std::string body = data.substr(headers_end);
                request->set_body(body);
            }
        }
        
        // Check if it's a WebSocket upgrade request
        if (request->is_websocket_request()) {
            Logger::getInstance().info("WebSocket upgrade request received, forwarding to WebSocket handler", "HttpServer");
            // In a real implementation, we would hand off to the WebSocket handler here
            auto response = std::make_shared<HttpResponse>(HttpStatus::BAD_REQUEST);
            response->set_body("WebSocket connections should be made to the WebSocket port", "text/plain");
            
            std::string response_str = response->to_string();
            boost::asio::write(*socket, boost::asio::buffer(response_str), error);
            socket->close();
            return;
        }
        
        // Process the request through the proxy handler
        auto response = proxy_handler_->handle_request(request, client_ip);
        
        // Send the response
        std::string response_str = response->to_string();
        boost::asio::write(*socket, boost::asio::buffer(response_str), error);
        
        // Close the connection
        socket->close();
        
        Logger::getInstance().debug("Connection from " + client_ip + " handled successfully", "HttpServer");
    }
    catch (const std::exception& e) {
        Logger::getInstance().error("Exception in connection handler: " + std::string(e.what()), "HttpServer");
        try {
            // Try to send an error response
            auto response = std::make_shared<HttpResponse>(HttpStatus::INTERNAL_SERVER_ERROR);
            response->set_body("Internal Server Error", "text/plain");
            
            std::string response_str = response->to_string();
            boost::system::error_code error;
            boost::asio::write(*socket, boost::asio::buffer(response_str), error);
            socket->close();
        }
        catch (...) {
            // If we can't send an error response, just close the socket
            try {
                socket->close();
            }
            catch (...) {}
        }
    }
}

HttpRequestPtr HttpServer::parse_request(const std::string& data) {
    std::istringstream stream(data);
    std::string line;
    
    // Parse request line
    if (!std::getline(stream, line)) {
        return nullptr;
    }
    
    // Remove CR from end of line if present
    if (!line.empty() && line.back() == '\r') {
        line.pop_back();
    }
    
    std::istringstream request_line(line);
    std::string method, uri, http_version;
    
    if (!(request_line >> method >> uri >> http_version)) {
        return nullptr;
    }
    
    auto request = std::make_shared<HttpRequest>(method, uri, http_version);
    
    // Parse headers
    while (std::getline(stream, line) && !line.empty()) {
        // Remove CR from end of line if present
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        
        // Empty line marks end of headers
        if (line.empty()) {
            break;
        }
        
        // Split header line at colon
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string name = line.substr(0, colon_pos);
            
            // Skip whitespace after colon
            size_t value_start = line.find_first_not_of(" \t", colon_pos + 1);
            std::string value;
            
            if (value_start != std::string::npos) {
                value = line.substr(value_start);
            }
            
            request->set_header(name, value);
        }
    }
    
    return request;
}