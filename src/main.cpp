#include "config/config.h"
#include "http/server.h"
#include <iostream>
#include <boost/asio.hpp>
#include <thread>
#include <signal.h>

#include "proxy/proxyHandler.h"
// #include "websocket/ws_server.h"
#include "util/logger.h"

// flag for shutdown
volatile bool running = true;

// signal handler for graceful shutdown
void signal_handler(int signal) {
    Logger::getInstance().info("Received signal " + std::to_string(signal) + ", shutting down...", "main.cpp");
    running = false;
}

int main(int argc, char* argv[]) {
    try {
        // parse command line arguments
        std::string config_path = "config/proxyConfig.json";
        if (argc > 1) {
            // manually set config path
            config_path = argv[1];
        }

        // initialize logger
        Logger::getInstance().init("logs", Logger::Level::INFO);
        Logger::getInstance().info("Starting reverse proxy...", "main.cpp");

        // register signal handlers for graceful shutdown
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);

        // load configuration
        Config config;
        if (!config.load(config_path)) {
            Logger::getInstance().error("Failed to load configuration from " + config_path, "main.cpp");
            return 1;
        }

        // Create IO context
        // it is like event dispatcher waits for events then calls the right handler function
        boost::asio::io_context io_context;

        // Initialize proxy handler
        // smart shared pointer 
        auto proxy_handler = std::make_shared<ProxyHandler>(config, io_context);
        
        // initialize HTTP server
        HttpServer server(io_context, config.get_http_port(), proxy_handler);
        
        // Initialize WebSocket server if enabled
        // std::unique_ptr<WebSocketServer> ws_server;
        // if (config.is_websocket_enabled()) {
        //     ws_server = std::make_unique<WebSocketServer>(
        //         config.get_websocket_port(), 
        //         proxy_handler
        //     );
        // }

        // Start servers
        server.start();
        // if (ws_server) {
        //     ws_server->start();
        // }

        Logger::getInstance().info("Reverse proxy started on HTTP port " + std::to_string(config.get_http_port()), "main.cpp");
        // if (ws_server) {
        //     Logger::getInstance().info("WebSocket server started on port " + std::to_string(config.get_websocket_port()), "main.cpp");
        // }

        // Create thread pool for the IO context
        unsigned int num_threads = std::thread::hardware_concurrency();
        std::vector<std::thread> threads;
        for (unsigned int i = 0; i < num_threads; ++i) {
            threads.emplace_back([&io_context]() {
                io_context.run();
            });
        }

        // Wait for shutdown signal
        while (running) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // Graceful shutdown
        Logger::getInstance().info("Shutting down servers...", "main.cpp");
        server.stop();
        // if (ws_server) {
        //     ws_server->stop();
        // }
        io_context.stop();

        // Wait for all threads to complete
        for (auto& thread : threads) {
            thread.join();
        }

        Logger::getInstance().info("Reverse proxy shutdown complete", "main.cpp");
        return 0;
    }
    catch (const std::exception& e) {
        Logger::getInstance().critical("Exception: " + std::string(e.what()), "main.cpp");
        return 1;
    }
}
