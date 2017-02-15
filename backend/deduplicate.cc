// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <ctime>
#include <iostream>
#include <fstream>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <cppdb/frontend.h>
#include <cppcms/json.h>

#include "parser/Parser.h"
#include "persistence/Persistence.h"

DEFINE_string(c, "", "backend configuration file to read database configuration");
DEFINE_int64(s, 0, "database ID of the statement to start with");

int main(int argc, char **argv) {
    google::gflags::ParseCommandLineFlags(&argc, &argv, true);
    google::gflags::SetUsageMessage(
            std::string("Deduplicates statements in the Primary Sources database. Usage:\n") +
            argv[0] + " -c config.json [-s start_id]");
    google::InitGoogleLogging(argv[0]);

    if (FLAGS_c == "") {
        google::gflags::ShowUsageWithFlags(argv[0]);
        std::cerr << "Option -c is required." << std::endl << std::endl;
        return 1;
    }

    // read configuration
    cppcms::json::value config;
    std::ifstream cfgfile(FLAGS_c);
    cfgfile >> config;

    try {
        clock_t begin = std::clock();

        std::cout << "DEDUPLICATE: open database connection" << std::endl;

        cppdb::session sql(
                wikidata::primarysources::build_connection(config["database"]));

        std::cout << "DEDUPLICATE: starting deduplication" << std::endl;

        sql.begin();
        wikidata::primarysources::Persistence p(sql, true);
        p.markDuplicates(FLAGS_s);
        sql.commit();

        std::cout << "DEDUPLICATE: deduplication finished" << std::endl;

        clock_t end = std::clock();
        std::cout << "DEDUPLICATE: timing "
                << 1000 * (static_cast<double>(end - begin) / CLOCKS_PER_SEC)
                << "ms" << std::endl;


        return 0;
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;

        return 1;
    }
}
