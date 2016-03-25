// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef TIME_LOGGER_H
#define TIME_LOGGER_H

#include <string>
#include <chrono>

namespace wikidata {
namespace primarysources {

/**
 * A time logger, writes a logging message when initialised and timing
 * information when destructed.
 */
class TimeLogger {
 public:
    TimeLogger(const std::string& message);

    ~TimeLogger();

    std::chrono::duration <double, std::milli> Elapsed();
 private:
    std::string message_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};

}  // namespace primarysources
}  // namespace wikidata

#endif //TIME_LOGGER_H
