#include "redis.h"
#include <stdexcept>
#include <iostream>

RedisClient::RedisClient(const std::string& host, int port, const std::string& password)
    : host_(host), port_(port), password_(password), context_(nullptr) {}

RedisClient::~RedisClient() {
    disconnect();
}

bool RedisClient::connect() {
    context_ = redisConnect(host_.c_str(), port_);
    if (context_ == nullptr || context_->err) {
        if (context_) {
            std::cerr << "Redis connection error: " << context_->errstr << std::endl;
            redisFree(context_);
        } else {
            std::cerr << "Redis connection error: cannot allocate redis context" << std::endl;
        }
        return false;
    }
    return authenticate();
}

void RedisClient::disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
}

bool RedisClient::authenticate() {
    if (!password_.empty()) {
        redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "AUTH %s", password_.c_str()));
        if (!reply || reply->type == REDIS_REPLY_ERROR) {
            std::cerr << "Redis authentication failed" << std::endl;
            if (reply) freeReplyObject(reply);
            return false;
        }
        freeReplyObject(reply);
    }
    return true;
}

std::string RedisClient::get(const std::string& key) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "GET %s", key.c_str()));
    if (!reply || reply->type != REDIS_REPLY_STRING) {
        if (reply) freeReplyObject(reply);
        return "";
    }
    std::string value = reply->str;
    freeReplyObject(reply);
    return value;
}

int RedisClient::get_int(const std::string& key) {
    std::string value = get(key);
    return value.empty() ? 0 : std::stoi(value);
}

void RedisClient::set(const std::string& key, const std::string& value) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "SET %s %s", key.c_str(), value.c_str()));
    if (reply) freeReplyObject(reply);
}

void RedisClient::set_with_expiry(const std::string& key, const std::string& value, int ttl_seconds) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "SETEX %s %d %s", key.c_str(), ttl_seconds, value.c_str()));
    if (reply) freeReplyObject(reply);
}

void RedisClient::increment(const std::string& key) {
    redisReply* reply = static_cast<redisReply*>(redisCommand(context_, "INCR %s", key.c_str()));
    if (reply) freeReplyObject(reply);
}
