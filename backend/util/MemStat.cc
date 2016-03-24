// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <fstream>
#include <unistd.h>

#include "MemStat.h"

wikidata::primarysources::MemStat::MemStat() {
    int tSize = 0, resident = 0, share = 0;
    std::ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
    buffer.close();

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    rss = resident * page_size_kb;
    shared_mem = share * page_size_kb;
    private_mem = rss - shared_mem;
}

