// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_SYSTEMSTATUS_H
#define PRIMARYSOURCES_SYSTEMSTATUS_H

#include <mutex>

#include <model/Status.h>
#include <persistence/Persistence.h>

namespace wikidata {
namespace primarysources {
namespace status {

class StatusService {
 public:
    // Initialise status. Sets up fields.
    StatusService(const std::string& connstr);

    void AddCacheHit();
    void AddCacheMiss();

    void AddGetEntityRequest();
    void AddGetRandomRequest();
    void AddGetStatementRequest();
    void AddUpdateStatementRequest();
    void AddGetStatusRequest();

    // Return the current system status.
    model::Status Status(const std::string& dataset = "");

    // Return the GIT version.
    std::string Version();

    void SetDirty() {
        dirty_ = true;
    }
 private:
    // SQL connection string.
    std::string connstr_;

    // Cached copy of the current status.
    model::Status status_;

    std::mutex status_mutex_;

    // True if the database status needs to be updated.
    bool dirty_;
};

void Init();


}
}
}

#endif //PRIMARYSOURCES_SYSTEMSTATUS_H
