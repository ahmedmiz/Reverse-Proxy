#pragma once

#include <string>
#include <memory>
#include "../config/config.h"

/**
 * Authentication class
 * Handles JWT token verification and authentication
 */
class Authentication {
public:
    /**
     * Constructor
     * @param config Application configuration
     */
    explicit Authentication(Config& config);
    
    /**
     * Verify a JWT token
     * @param token JWT token to verify
     * @return True if token is valid
     */
    bool verify_jwt(const std::string& token);
    
    /**
     * Generate a JWT token for testing
     * @param subject Subject identifier
     * @param expiry_seconds Seconds until token expires
     * @return Generated JWT token
     */
    std::string generate_jwt(const std::string& subject, int expiry_seconds = 3600);

private:
    Config& config_;
    std::string secret_;
    
    /**
     * Base64 URL decode a string
     * @param input Base64 URL encoded string
     * @return Decoded string
     */
    std::string base64_url_decode(const std::string& input);
    
    /**
     * Base64 URL encode a string
     * @param input String to encode
     * @return Base64 URL encoded string
     */
    std::string base64_url_encode(const std::string& input);
    
    /**
     * Parse and validate JWT payload
     * @param payload_json JSON payload string
     * @return True if payload is valid
     */
    bool validate_payload(const std::string& payload_json);
};