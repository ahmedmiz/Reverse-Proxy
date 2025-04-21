#pragma once

#include <boost/asio.hpp>
#include <memory>
#include <string>
#include "RequestHandler.h"
#include "ResponseHandler.h"
#include "../proxy/proxyHandler.h"

// Forward declarations
class ProxyHandler;

/**
 * HTTP Server class
 * Handles incoming HTTP connections, parses requests, and sends responses
 */
class HttpServer
{
public:
    /**
     * Constructor
     * @param io_context Boost asio io_context
     * @param port Port to listen on
     * @param proxy_handler Handler for proxying requests
     */
    HttpServer(boost::asio::io_context &io_context, int port,
               std::shared_ptr<ProxyHandler> proxy_handler);

    /**
     * Start the server
     */
    void start();

    /**
     * Stop the server
     */
    void stop();

private:
    boost::asio::io_context &io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<ProxyHandler> proxy_handler_;
    bool running_;

    /**
     * Start accepting connections
     */
    void start_accept();

    /**
     * Handle a new connection
     * @param socket Socket for the new connection
     * @param error Error code
     */
    void handle_accept(std::shared_ptr<boost::asio::ip::tcp::socket> socket,
                       const boost::system::error_code &error);

    /**
     * Handle a new connection
     * @param socket Socket for the connection
     */
    void handle_connection(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    /**
     * Parse an HTTP request from data
     * @param data Raw HTTP request data
     * @return Parsed HTTP request or nullptr if invalid
     */
    HttpRequestPtr parse_request(const std::string &data);
};
