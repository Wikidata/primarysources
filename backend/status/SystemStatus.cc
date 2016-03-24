// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <util/MemStat.h>
#include <status/Version.h>
#include <ctime>
#include <mutex>

#include "SystemStatus.h"


namespace wikidata {
namespace primarysources {
namespace status {

namespace {
// A global status object that is shared between all threads and classes.
model::Status g_status;

std::mutex g_status_mutex;

// format a time_t using ISO8601 GMT time
inline std::string formatGMT(time_t* time) {
    char result[128];
    std::strftime(result, 128, "%Y-%m-%dT%H:%M:%SZ", gmtime(time));
    return std::string(result);
}

}  // namespace


void Init() {
    // set system startup time
    time_t startupTime = std::time(nullptr);
    g_status.mutable_system()->set_startup(formatGMT(&startupTime));
    g_status.mutable_system()->set_version(std::string(GIT_SHA1));
}

void AddCacheHit() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_system()->set_cache_hits(
            g_status.system().cache_hits() + 1);
}
void AddCacheMiss() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_system()->set_cache_misses(
            g_status.system().cache_misses() + 1);
}

void AddGetEntityRequest() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_requests()->set_get_entity(
            g_status.requests().get_entity() + 1);
}

void AddGetRandomRequest() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_requests()->set_get_random(
            g_status.requests().get_random() + 1);
}

void AddGetStatementRequest() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_requests()->set_get_statement(
            g_status.requests().get_statement() + 1);

}

void AddUpdateStatementRequest() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_requests()->set_update_statement(
            g_status.requests().update_statement() + 1);
}

void AddGetStatusRequest() {
    std::lock_guard<std::mutex> lock(g_status_mutex);
    g_status.mutable_requests()->set_get_status(
            g_status.requests().get_status() + 1);
}

// Update the system status and return a constant reference.
model::Status Status() {
    std::lock_guard<std::mutex> lock(g_status_mutex);

    MemStat memstat;
    g_status.mutable_system()->set_shared_memory(memstat.getSharedMem());
    g_status.mutable_system()->set_private_memory(memstat.getPrivateMem());
    g_status.mutable_system()->set_resident_set_size(memstat.getRSS());

    return g_status;
}

std::string Version() {
   return std::string(GIT_SHA1);
}

}  // namespace status
}  // namespace primarysources
}  // namespace wikidata
