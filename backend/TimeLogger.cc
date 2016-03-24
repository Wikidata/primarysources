//
// Created by wastl on 13.02.16.
//

#include <booster/log.h>
#include <iostream>
#include "TimeLogger.h"

namespace wikidata {
namespace primarysources {

TimeLogger::TimeLogger(const std::string &message)
        : start_(std::chrono::steady_clock::now())
        , message_(message) { }

TimeLogger::~TimeLogger() {
    BOOSTER_NOTICE("sourcestool")
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
