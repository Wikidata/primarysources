// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_REDISCACHESERVICE_H
#define PRIMARYSOURCES_REDISCACHESERVICE_H

#include <redox.hpp>
#include <google/protobuf/message.h>

namespace wikidata {
namespace primarysources {

// A cache service backed by a Redis server. Allows storing string and
// proto message values. Keys are prefixed with a primary sources tool
// specific prefix that can be overridden in the constructor.
class RedisCacheService {
 public:
    RedisCacheService(const std::string& host, int port,
                      const std::string& prefix = "PST::");

    ~RedisCacheService();

    // Add a key/value pair to the cache. The key will be prefixed by the
    // shared prefix used in this service.
    void Add(const std::string& key, const std::string& value);
    void Add(const std::string& key, const google::protobuf::Message& value);

    // Retrieve a string or protobuf value by key. The key will be prefixed by the
    // shared prefix used in this service.
    bool Get(const std::string& key, std::string* result);
    bool Get(const std::string& key, google::protobuf::Message* result);

    // Remove the value with the given key from the cache. The key will be
    // prefixed by the shared prefix used in this service.
    void Evict(const std::string& key);
 private:
    redox::Redox redox_;

    std::string prefix_;
};


class CacheException : public std::exception {
 public:
    explicit CacheException(const std::string &message)
            : message(message) { }

    const char *what() const noexcept override;

 private:
    std::string message;
};



}  // namespace primarysources
}  // namespace wikidata

#endif //PRIMARYSOURCES_REDISCACHESERVICE_H
