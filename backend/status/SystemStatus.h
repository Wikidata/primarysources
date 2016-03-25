// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_SYSTEMSTATUS_H
#define PRIMARYSOURCES_SYSTEMSTATUS_H

#include <model/status.pb.h>

namespace wikidata {
namespace primarysources {
namespace status {

// Initialise status. Sets up static fields.
void Init();

void AddCacheHit();
void AddCacheMiss();

void AddGetEntityRequest();
void AddGetRandomRequest();
void AddGetStatementRequest();
void AddUpdateStatementRequest();
void AddGetStatusRequest();

// Return the current system status.
model::Status Status();

// Return the GIT version.
std::string Version();
}
}
}

#endif //PRIMARYSOURCES_SYSTEMSTATUS_H
