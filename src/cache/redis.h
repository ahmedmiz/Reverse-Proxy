#pragma once

#include <string>
#include <memory>
#include <hiredis/hiredis.h>

class RedisClient {
public:
    RedisClient(const std::string& host, int port, const std::string& password = "");
    ~RedisClient();

    bool connect();
    void disconnect();

    std::string get(const std::string& key);
    int get_int(const std::string& key);
    void set(const std::string& key, const std::string& value);
    void set_with_expiry(const std::string& key, const std::string& value, int ttl_seconds);
    void increment(const std::string& key);

private:
    std::string host_;
    int port_;
    std::string password_;
    redisContext* context_;

    bool authenticate();
};
