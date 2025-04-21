#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "../src/config/Config.h"

TEST_CASE("Config loading from JSON") {
    Config config;

    SUBCASE("Valid configuration file") {
        REQUIRE(config.load("../config/proxyConfig.json") == true);
        CHECK(config.get_http_port() == 8080);
        CHECK(config.is_websocket_enabled() == true);
        CHECK(config.get_websocket_port() == 8081);
        CHECK(config.is_ssl_enabled() == true);
        CHECK(config.get_ssl_cert_path() == "/etc/ssl/certs/fullchain.pem");
        CHECK(config.get_ssl_key_path() == "/etc/ssl/private/privkey.pem");
    }

    SUBCASE("Invalid configuration file") {
        REQUIRE(config.load("../config/invalidConfig.json") == false);
    }
}