// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <glog/logging.h>
#include <iostream>
#include "TimeLogger.h"

namespace wikidata {
namespace primarysources {

TimeLogger::TimeLogger(const std::string &message)
        : start_(std::chrono::steady_clock::now())
        , message_(message) { }

TimeLogger::~TimeLogger() {
    LOG(INFO)
        << message_ << " time: "
        << std::chrono::duration <double, std::milli> (
            std::chrono::steady_clock::now() - start_).count()
        << "ms" << std::endl;
}

std::chrono::duration<double, std::milli> TimeLogger::Elapsed() {
    return std::chrono::duration <double, std::milli> (
            std::chrono::steady_clock::now() - start_);
}

}  // namespace primarysources
}  // namespace wikidata
