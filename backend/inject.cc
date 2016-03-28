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
#include <chrono>

#include "parser/Parser.h"
#include "persistence/Persistence.h"

using wikidata::primarysources::model::Statement;
using wikidata::primarysources::parser::parseTSV;

void usage(char *cmd) {
    std::cout << "Usage: " << cmd << " [-z] -c config.json -i datafile -d dataset" << std::endl;
    std::cout << "Options:" <<std::endl;
    std::cout << " -c config.json     backend configuration file to read database configuration" << std::endl;
    std::cout << " -i datafile        input data file containing statements in TSV format" << std::endl;
    std::cout << " -d dataset         name of the dataset we are importing (e,g, \"freebase\")" << std::endl;
    std::cout << " -z                 input is compressed with GZIP" << std::endl;
}

int main(int argc, char **argv) {
    int opt;
    bool gzipped = false;
    std::string datafile, configfile, dataset;

    // read options from command line
    while( (opt = getopt(argc,argv,"zc:i:d:")) != -1) {
        switch(opt) {
            case 'c':
                configfile = optarg;
                break;
            case 'i':
                datafile = optarg;
                break;
            case 'd':
                dataset = optarg;
                break;
            case 'z':
                gzipped = true;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (datafile == "" || configfile == "" || dataset == "") {
        std::cerr << "Options -d, -c and -i are required." << std::endl << std::endl;
        usage(argv[0]);
        return 1;
    }

    // read configuration
    cppcms::json::value config;
    std::ifstream cfgfile(configfile);
    cfgfile >> config;

    try {

        std::ifstream file(datafile, std::ios_base::in | std::ios_base::binary);

        if(file.fail()) {
            std::cerr << "could not open data file" << std::endl;
            return 1;
        }

        boost::iostreams::filtering_istreambuf zin;
        if (gzipped) {
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
        parseTSV(dataset, upload, in, [&sql, &p, &count](Statement st) {
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
