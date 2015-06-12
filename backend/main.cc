// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolService.h"
#include "Version.h"

#include <cppcms/applications_pool.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <booster/log.h>

#ifdef DB_SQLITE3
#include <sqlite3.h>
#endif

int main(int argc, char **argv) {
    try {

#ifdef DB_SQLITE3
        // turn on shared process cache
        sqlite3_enable_shared_cache(true);
#endif

        cppcms::service srv(argc, argv);
        srv.applications_pool().mount(
                cppcms::applications_factory<SourcesToolService>()
        );

        BOOSTER_NOTICE("sourcestool") <<
                "Initialising Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") ..." << std::endl;

        srv.run();

        BOOSTER_NOTICE("sourcestool") <<
                "Shutting down Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") ..." << std::endl;

        return 0;
    }
    catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;

        BOOSTER_NOTICE("sourcestool") <<
                "Terminating Wikidata SourcesTool (Version: " <<
                GIT_SHA1 << ") on Exception" << std::endl;

        return 1;
    }
}
