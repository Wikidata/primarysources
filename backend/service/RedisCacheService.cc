// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "RedisCacheService.h"

#include <glog/logging.h>

namespace wikidata {
namespace primarysources {

RedisCacheService::RedisCacheService(
        const std::string &host, int port, const std::string &prefix)
        : prefix_(prefix), host_(host), port_(port), connected_(false) {
    LOG(INFO) << "Initialised Redis service (host=" << host << ", port=" << port << ")";
}

RedisCacheService::~RedisCacheService() {
    std::lock_guard<std::mutex> lock(redox_mutex_);
    if (connected_) {
        redox_->disconnect();
    }
    LOG(INFO) << "Shutdown Redis service";
}


void RedisCacheService::Add(const std::string &key, const std::string &value) {
    Connect();

    if (!redox_->set(prefix_ + key, value)) {
        throw CacheException("Could not store key/value on Redis server");
    };
}

void RedisCacheService::Add(const std::string &key, const google::protobuf::Message &value) {
    std::string v = value.SerializeAsString();
    Add(key, v);
}

bool RedisCacheService::Get(const std::string &key, std::string *result) {
    Connect();

    auto& c = redox_->commandSync<std::string>({"GET", prefix_ + key});
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
    Connect();
    redox_->del(prefix_ + key);
}

void RedisCacheService::Connect() {
    if (connected_) {
        return;
    }

    std::lock_guard<std::mutex> lock(redox_mutex_);
    redox_.reset(new redox::Redox(LOG(INFO)));

    connected_ = redox_->connect(host_, port_, [this](int state) {
        connected_ = state == redox::Redox::CONNECTED;
    });

    if (!connected_) {
        throw CacheException("Could not connect to Redis server");
    }
}


const char* CacheException::what() const noexcept {
    return message.c_str();
}

}  // namespace primarysources
}  // namespace wikidata



