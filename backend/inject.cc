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
#include <boost/iostreams/filter/gzip.hpp>
#include <cppdb/frontend.h>
#include <cppcms/json.h>
#include <chrono>

#include "parser/Parser.h"
#include "persistence/Persistence.h"

DEFINE_string(c, "", "backend configuration file to read database configuration");
DEFINE_string(i, "", "input data file containing statements in TSV format");
DEFINE_string(d, "", "name of the dataset we are importing (e,g, \"freebase\")");
DEFINE_bool(z, false, "input is compressed with GZIP");

using wikidata::primarysources::model::Statement;
using wikidata::primarysources::parser::parseTSV;

int main(int argc, char **argv) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    gflags::SetUsageMessage(
            std::string("Import statements into the Primary Sources database. Sample Usage: \n") +
            argv[0] + " [-z] -c config.json -i datafile -d dataset");
    google::InitGoogleLogging(argv[0]);

    if (FLAGS_i == "" || FLAGS_c == "" || FLAGS_d == "") {
        gflags::ShowUsageWithFlags(argv[0]);
        std::cerr << "Options -d, -c and -i are required." << std::endl << std::endl;
        return 1;
    }

    // read configuration
    cppcms::json::value config;
    std::ifstream cfgfile(FLAGS_c);
    cfgfile >> config;

    try {

        std::ifstream file(FLAGS_i, std::ios_base::in | std::ios_base::binary);

        if(file.fail()) {
            std::cerr << "could not open data file" << std::endl;
            return 1;
        }

        boost::iostreams::filtering_istreambuf zin;
        if (FLAGS_z) {
            zin.push(boost::iostreams::gzip_decompressor());
        }
        zin.push(file);

        std::istream in(&zin);

        clock_t begin = std::clock();

        cppdb::session sql(
                wikidata::primarysources::build_connection(config["database"]));

        sql.begin();
        wikidata::primarysources::Persistence p(sql, true);

        // get timestamp and use it as upload id
        int64_t upload = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();

        int64_t count = 0;
        parseTSV(FLAGS_d, upload, in, [&sql, &p, &count](Statement st) {
            try {
                p.addStatement(st);
            } catch (wikidata::primarysources::PersistenceException& e) {
                std::cerr << "Skipping statement because of error: " << e.what() << std::endl;
            }
            count++;

            // batch commit
            if(count % 100000 == 0) {
                sql.commit();

                std::cout << "processed " << count << " statements ..." << std::endl;

                sql.begin();
            }
        });
        sql.commit();

        clock_t end = std::clock();
        std::cout << "injection time (" << count << " statements): "
                << 1000 * (static_cast<double>(end - begin) / CLOCKS_PER_SEC)
                << "ms" << std::endl;


        return 0;
    } catch (std::exception const &e) {
        std::cerr << e.what() << std::endl;

        return 1;
    }
}
