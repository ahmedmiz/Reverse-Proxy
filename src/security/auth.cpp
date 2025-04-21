#include "auth.h"
#include "../util/logger.h"
#include <json/json.h>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/sha.h>
#include <chrono>
#include <vector>
#include <sstream>

Authentication::Authentication(Config& config) : config_(config) {
    secret_ = config.get_jwt_secret();
    
    if (secret_.empty()) {
        Logger::getInstance().error("JWT secret is empty", "Auth");
    }
}

bool Authentication::verify_jwt(const std::string& token) {
    try {
        // Split token into parts
        size_t first_dot = token.find('.');
        size_t second_dot = token.find('.', first_dot + 1);
        
        if (first_dot == std::string::npos || second_dot == std::string::npos) {
            Logger::getInstance().warning("Invalid JWT format - missing dots", "Auth");
            return false;
        }
        
        std::string header_b64 = token.substr(0, first_dot);
        std::string payload_b64 = token.substr(first_dot + 1, second_dot - first_dot - 1);
        std::string signature_b64 = token.substr(second_dot + 1);
        
        // Verify signature
        std::string data_to_sign = header_b64 + "." + payload_b64;
        
        // Calculate HMAC-SHA256
        unsigned char hmac[EVP_MAX_MD_SIZE];
        unsigned int hmac_len;
        
        HMAC(EVP_sha256(), secret_.c_str(), secret_.length(),
             reinterpret_cast<const unsigned char*>(data_to_sign.c_str()),
             data_to_sign.length(), hmac, &hmac_len);
        
        // Base64 URL encode the HMAC
        std::string computed_signature_b64 = base64_url_encode(
            std::string(reinterpret_cast<char*>(hmac), hmac_len)
        );
        
        // Compare signatures
        if (computed_signature_b64 != signature_b64) {
            Logger::getInstance().warning("JWT signature verification failed", "Auth");
            return false;
        }
        
        // Decode payload
        std::string payload_json = base64_url_decode(payload_b64);
        
        // Validate payload contents (expiry, etc.)
        return validate_payload(payload_json);
    }
    catch (const std::exception& e) {
        Logger::getInstance().error("Exception during JWT verification: " + std::string(e.what()), "Auth");
        return false;
    }
}

std::string Authentication::generate_jwt(const std::string& subject, int expiry_seconds) {
    try {
        // Current time and expiry time
        auto now = std::chrono::system_clock::now();
        auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        auto exp = now_sec + expiry_seconds;
        
        // Create header
        Json::Value header;
        header["alg"] = "HS256";
        header["typ"] = "JWT";
        
        // Create payload
        Json::Value payload;
        payload["sub"] = subject;
        payload["iat"] = static_cast<Json::Value::Int64>(now_sec);
        payload["exp"] = static_cast<Json::Value::Int64>(exp);
        
        // Serialize to JSON strings
        Json::FastWriter writer;
        std::string header_json = writer.write(header);
        std::string payload_json = writer.write(payload);
        
        // Remove newlines added by FastWriter
        if (!header_json.empty() && header_json.back() == '\n') {
            header_json.pop_back();
        }
        if (!payload_json.empty() && payload_json.back() == '\n') {
            payload_json.pop_back();
        }
        
        // Base64 URL encode
        std::string header_b64 = base64_url_encode(header_json);
        std::string payload_b64 = base64_url_encode(payload_json);
        
        // Data to sign
        std::string data_to_sign = header_b64 + "." + payload_b64;
        
        // Calculate HMAC-SHA256
        unsigned char hmac[EVP_MAX_MD_SIZE];
        unsigned int hmac_len;
        
        HMAC(EVP_sha256(), secret_.c_str(), secret_.length(),
             reinterpret_cast<const unsigned char*>(data_to_sign.c_str()),
             data_to_sign.length(), hmac, &hmac_len);
        
        // Base64 URL encode the HMAC
        std::string signature_b64 = base64_url_encode(
            std::string(reinterpret_cast<char*>(hmac), hmac_len)
        );
        
        // Assemble JWT
        return data_to_sign + "." + signature_b64;
    }
    catch (const std::exception& e) {
        Logger::getInstance().error("Exception during JWT generation: " + std::string(e.what()), "Auth");
        return "";
    }
}

std::string Authentication::base64_url_decode(const std::string& input) {
    // Convert base64url to base64
    std::string base64 = input;
    std::replace(base64.begin(), base64.end(), '-', '+');
    std::replace(base64.begin(), base64.end(), '_', '/');
    
    // Add padding if needed
    switch (base64.size() % 4) {
        case 0: break;
        case 2: base64 += "=="; break;
        case 3: base64 += "="; break;
        default: throw std::runtime_error("Illegal base64url string");
    }
    
    // Decode base64
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    
    BIO* bmem = BIO_new_mem_buf(base64.c_str(), base64.length());
    bmem = BIO_push(b64, bmem);
    
    std::vector<char> buffer(base64.size());
    int decoded_size = BIO_read(bmem, buffer.data(), buffer.size());
    
    BIO_free_all(bmem);
    
    if (decoded_size <= 0) {
        throw std::runtime_error("Failed to decode base64url");
    }
    
    return std::string(buffer.data(), decoded_size);
}

std::string Authentication::base64_url_encode(const std::string& input) {
    BIO* bmem = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    bmem = BIO_push(b64, bmem);
    
    BIO_write(bmem, input.c_str(), input.length());
    BIO_flush(bmem);
    
    BUF_MEM* bptr;
    BIO_get_mem_ptr(bmem, &bptr);
    
    std::string result(bptr->data, bptr->length);
    
    BIO_free_all(bmem);
    
    // Convert base64 to base64url
    std::replace(result.begin(), result.end(), '+', '-');
    std::replace(result.begin(), result.end(), '/', '_');
    
    // Remove padding
    while (!result.empty() && result.back() == '=') {
        result.pop_back();
    }
    
    return result;
}

bool Authentication::validate_payload(const std::string& payload_json) {
    Json::CharReaderBuilder reader;
    Json::Value payload;
    std::string errors;
    
    std::istringstream stream(payload_json);
    if (!Json::parseFromStream(reader, stream, &payload, &errors)) {
        Logger::getInstance().warning("Failed to parse JWT payload: " + errors, "Auth");
        return false;
    }
    
    // Check expiry time
    if (!payload.isMember("exp") || !payload["exp"].isInt64()) {
        Logger::getInstance().warning("JWT missing expiry time", "Auth");
        return false;
    }
    
    Json::Value::Int64 exp = payload["exp"].asInt64();
    auto now = std::chrono::system_clock::now();
    auto now_sec = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
    
    if (now_sec > exp) {
        Logger::getInstance().warning("JWT has expired", "Auth");
        return false;
    }
    
    // Add additional validation as needed
    
    return true;
}
