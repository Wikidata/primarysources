// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef MEMSTAT_H
#define MEMSTAT_H

namespace wikidata {
namespace primarysources {

// Collect memory statistics and make them accessible.
class MemStat {
 public:
    MemStat();

    double getRSS() const {
        return rss;
    }

    double getSharedMem() const {
        return shared_mem;
    }

    double getPrivateMem() const {
        return private_mem;
    }

 private:
    double rss, shared_mem, private_mem;
};

}  // namespace primarysources
}  // namespace wikidata

#endif  // MEMSTAT_H
