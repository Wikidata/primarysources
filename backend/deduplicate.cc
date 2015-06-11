// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <ctime>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <cppdb/frontend.h>
#include <cppcms/json.h>

#include "Parser.h"
#include "Persistence.h"

std::string build_connection(const cppcms::json::value& config) {
    std::ostringstream out;
    out << config["driver"].str() << ":db=" << config["name"].str();
    if (config["host"].str() != "") {
        out << ";host=" << config["host"].str();
    }
    if (config["port"].str() != "") {
        out << ";port=" << config["port"].str();
    }
    if (config["user"].str() != "") {
        out << ";user=" << config["user"].str();
    }
    if (config["password"].str() != "") {
        out << ";pass=" << config["password"].str();
    }
    return out.str();
}


void usage(char *cmd) {
    std::cout << "Usage: " << cmd << " -c config.json [-s start_id]" << std::endl;
    std::cout << "Options:" <<std::endl;
    std::cout << " -c config.json     backend configuration file to read database configuration" << std::endl;
}

int main(int argc, char **argv) {
    int opt;
    int64_t start_id = 0;
    std::string configfile;

    // read options from command line
    while( (opt = getopt(argc,argv,"c:s:")) != -1) {
        switch(opt) {
            case 'c':
                configfile = optarg;
                break;
            case 's':
                start_id = atoi(optarg);
                break;

            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (configfile == "") {
        std::cerr << "Option -c is required." << std::endl << std::endl;
        usage(argv[0]);
        return 1;
    }

    // read configuration
    cppcms::json::value config;
    std::ifstream cfgfile(configfile);
    cfgfile >> config;

    try {
        clock_t begin = std::clock();

        std::cout << "DEDUPLICATE: open database connection" << std::endl;

        cppdb::session sql(build_connection(config["database"]));

        std::cout << "DEDUPLICATE: starting deduplication" << std::endl;

        sql.begin();
        Persistence p(sql, true);
        p.markDuplicates(start_id);
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
