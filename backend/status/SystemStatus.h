// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_SYSTEMSTATUS_H
#define PRIMARYSOURCES_SYSTEMSTATUS_H

#include <thread>
#include <condition_variable>
#include <mutex>

#include <model/Status.h>

namespace wikidata {
namespace primarysources {
namespace status {

/**
 * Provide system status. Should be used as a singleton using the instance() method.
 */
class StatusService {
 public:

    void AddCacheHit();
    void AddCacheMiss();
    void AddRedisHit();
    void AddRedisMiss();

    void AddGetEntityRequest();
    void AddGetRandomRequest();
    void AddGetStatementRequest();
    void AddUpdateStatementRequest();
    void AddGetStatusRequest();

    // Return the current system status.
    model::Status Status(const std::string& dataset = "");

    // Return the GIT version.
    std::string Version() const;

    void SetDirty() {
        std::unique_lock<std::mutex> lck(status_mutex_);
        dirty_ = true;
        notify_dirty_.notify_one();
    }

    static StatusService& instance(const std::string& connstr) {
        static StatusService instance_(connstr);
        return instance_;
    }
 private:
    // Initialise status. Sets up fields.
    StatusService(const std::string& connstr);

    ~StatusService() {
        std::unique_lock<std::mutex> lck(status_mutex_);
        shutdown_ = true;
        notify_dirty_.notify_one();
    };

    void Update();

    // SQL connection string.
    std::string connstr_;

    // Updater thread, fetches new data in the background when dirty is true;
    std::thread updater_;

    // Conditional variable to notify when dirty is set. Wakes up updater thread.
    std::condition_variable notify_dirty_;

    // Cached copy of the current status.
    model::Status status_;

    std::mutex status_mutex_;
    std::mutex query_mutex_;

    // True if the database status needs to be updated.
    bool dirty_;

    bool shutdown_;
};

void Init();


}
}
}

#endif //PRIMARYSOURCES_SYSTEMSTATUS_H
