// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "RedisCacheService.h"

namespace wikidata {
namespace primarysources {

RedisCacheService::RedisCacheService(
        const std::string &host, int port, const std::string &prefix)
        : prefix_(prefix) {
    if (!redox_.connect(host, port)) {
        throw CacheException("Could not connect to Redis server!");
    }
}

void RedisCacheService::Add(const std::string &key, const std::string &value) {
    if (!redox_.set(prefix_ + key, value)) {
        throw CacheException("Could not store key/value on Redis server");
    };
}

void RedisCacheService::Add(const std::string &key, const google::protobuf::Message &value) {
    std::string v = value.SerializeAsString();
    Add(key, v);
}

bool RedisCacheService::Get(const std::string &key, std::string *result) {
    auto& c = redox_.commandSync<std::string>({"GET", prefix_ + key});
    if (!c.ok()) {
        c.free();
        return false;
    }

    *result = c.reply();
    c.free();

    return true;
}

bool RedisCacheService::Get(const std::string &key, google::protobuf::Message *result) {
    std::string v;
    if (!Get(key, &v)) {
        return false;
    }

    result->ParseFromString(v);
    return true;
}

void RedisCacheService::Evict(const std::string &key) {
    redox_.del(prefix_ + key);
}


const char* CacheException::what() const noexcept {
    return message.c_str();
}

}  // namespace primarysources
}  // namespace wikidata



