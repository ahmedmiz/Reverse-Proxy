#include "ssl.h"
#include "../util/logger.h"
#include <boost/asio/ssl/context.hpp>
#include <fstream>

SSLContextManager::SSLContextManager(Config& config)
    : config_(config), enabled_(config.is_ssl_enabled()) {
    
    if (enabled_) {
        initialize_context();
    }
}

boost::asio::ssl::context& SSLContextManager::get_context() {
    if (!ssl_context_) {
        throw std::runtime_error("SSL context not initialized");
    }
    
    return *ssl_context_;
}

bool SSLContextManager::is_enabled() const {
    return enabled_;
}

void SSLContextManager::initialize_context() {
    try {
        // Create SSL context
        ssl_context_ = std::make_unique<boost::asio::ssl::context>(
            boost::asio::ssl::context::sslv23_server
        );
        
        // Set options
        ssl_context_->set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::no_sslv3 |
            boost::asio::ssl::context::no_tlsv1 |
            boost::asio::ssl::context::single_dh_use
        );
        
        // Set certificate file
        ssl_context_->use_certificate_chain_file(config_.get_ssl_cert_path());
        
        // Set private key file
        ssl_context_->use_private_key_file(
            config_.get_ssl_key_path(),
            boost::asio::ssl::context::pem
        );
        
        Logger::getInstance().info("SSL context initialized successfully", "SSL");
    }
    catch (const std::exception& e) {
        Logger::getInstance().error("Failed to initialize SSL context: " + std::string(e.what()), "SSL");
        throw;  // Re-throw to prevent starting server with invalid SSL settings
    }
}
