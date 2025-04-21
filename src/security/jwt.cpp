#include "jwt.h"
#include <iostream>
#include <sstream>
#include <chrono>
#include <ctime>
#include <openssl/hmac.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdexcept>
#include <json/json.h>

JWT::JWT() : isValid_(false) {}

JWT::~JWT() {}

std::string JWT::generateToken(const std::map<std::string, std::string>& payload, 
                               const std::string& secret, 
                               long expirationTime) {
    Json::Value header;
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    Json::Value payloadJson;
    for (const auto& [key, value] : payload) {
        payloadJson[key] = value;
    }

    // Add expiration time
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    payloadJson["exp"] = static_cast<Json::Int64>(now + expirationTime);

    // Encode header and payload
    std::string encodedHeader = base64Encode(header.toStyledString());
    std::string encodedPayload = base64Encode(payloadJson.toStyledString());

    // Generate signature
    std::string signature = generateSignature(encodedHeader, encodedPayload, secret);

    // Combine all parts
    return encodedHeader + "." + encodedPayload + "." + signature;
}

bool JWT::verifyToken(const std::string& token, const std::string& secret) {
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);

    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }

    std::string encodedHeader = token.substr(0, firstDot);
    std::string encodedPayload = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string providedSignature = token.substr(secondDot + 1);

    // Verify signature
    std::string expectedSignature = generateSignature(encodedHeader, encodedPayload, secret);
    if (providedSignature != expectedSignature) {
        return false;
    }

    // Decode payload and check expiration
    std::string decodedPayload = base64Decode(encodedPayload);
    Json::Value payloadJson;
    Json::CharReaderBuilder readerBuilder;
    std::string errs;
    std::istringstream stream(decodedPayload);
    if (!Json::parseFromStream(readerBuilder, stream, &payloadJson, &errs)) {
        return false;
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    if (payloadJson.isMember("exp") && payloadJson["exp"].asInt64() < now) {
        return false;
    }

    // Store payload and mark as valid
    payload_.clear();
    for (const auto& key : payloadJson.getMemberNames()) {
        payload_[key] = payloadJson[key].asString();
    }
    isValid_ = true;

    return true;
}

std::map<std::string, std::string> JWT::getPayload() const {
    if (!isValid_) {
        throw std::runtime_error("Token is not valid");
    }
    return payload_;
}

bool JWT::isExpired() const {
    if (!isValid_) {
        throw std::runtime_error("Token is not valid");
    }

    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto exp = std::stol(payload_.at("exp"));
    return now > exp;
}

std::string JWT::base64Encode(const std::string& input) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, input.data(), input.size());
    BIO_flush(bio);

    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string encoded(bufferPtr->data, bufferPtr->length);

    BIO_free_all(bio);
    return encoded;
}

std::string JWT::base64Decode(const std::string& input) {
    BIO* bio = BIO_new_mem_buf(input.data(), input.size());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);

    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    char buffer[input.size()];
    int decodedSize = BIO_read(bio, buffer, input.size());

    BIO_free_all(bio);
    return std::string(buffer, decodedSize);
}

std::string JWT::generateSignature(const std::string& header, 
                                   const std::string& payload, 
                                   const std::string& secret) {
    std::string data = header + "." + payload;

    unsigned char* digest = HMAC(EVP_sha256(), secret.data(), secret.size(),
                                 reinterpret_cast<const unsigned char*>(data.data()), data.size(), nullptr, nullptr);

    return base64Encode(std::string(reinterpret_cast<char*>(digest), 32));
}
