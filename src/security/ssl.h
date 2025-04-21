#pragma once

#include <string>
#include <memory>
#include <boost/asio/ssl.hpp>
#include "../config/config.h"

/**
 * SSL Context Manager
 * Creates and configures an SSL context for the server
 */
class SSLContextManager {
public:
    /**
     * Constructor
     * @param config Application configuration
     */
    explicit SSLContextManager(Config& config);
    
    /**
     * Get the SSL context
     * @return SSL context
     */
    boost::asio::ssl::context& get_context();
    
    /**
     * Check if SSL is enabled
     * @return True if SSL is enabled
     */
    bool is_enabled() const;

private:
    Config& config_;
    std::unique_ptr<boost::asio::ssl::context> ssl_context_;
    bool enabled_;
    
    /**
     * Initialize the SSL context with certificates and settings
     */
    void initialize_context();
};
