// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "service/SourcesToolService.h"
#include "status/Version.h"

#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <status/SystemStatus.h>
#include <gflags/gflags.h>
#include <glog/logging.h>

#ifdef DB_SQLITE3
#include <sqlite3.h>
#endif

using wikidata::primarysources::SourcesToolService;

int main(int argc, char **argv) {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    google::ParseCommandLineFlags(&argc, &argv, true);

    wikidata::primarysources::status::Init();
    try {

#ifdef DB_SQLITE3
        // turn on shared process cache
        sqlite3_enable_shared_cache(true);
#endif

        cppcms::service srv(argc, argv);
        srv.applications_pool().mount(
                cppcms::applications_factory<SourcesToolService>()
        );

        LOG(INFO) <<
                "Initialising Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") ..." << std::endl;

        srv.run();

        LOG(INFO) <<
                "Shutting down Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") ..." << std::endl;

        return 0;
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;

        LOG(INFO) <<
                "Terminating Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") on Exception" << std::endl;

        return 1;
    }
}
