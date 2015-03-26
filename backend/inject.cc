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
    std::cout << "Usage: " << cmd << " [-z] -c config.json -i datafile" << std::endl;
    std::cout << "Options:" <<std::endl;
    std::cout << " -c config.json     backend configuration file to read database configuration" << std::endl;
    std::cout << " -i datafile        input data file containing statements in TSV format" << std::endl;
    std::cout << " -z                 input is compressed with GZIP" << std::endl;
}

int main(int argc, char **argv) {
    int opt;
    bool gzipped = false;
    std::string datafile, configfile;

    // read options from command line
    while( (opt = getopt(argc,argv,"zc:i:")) != -1) {
        switch(opt) {
            case 'c':
                configfile = optarg;
                break;
            case 'i':
                datafile = optarg;
                break;
            case 'z':
                gzipped = true;
                break;
            default:
                usage(argv[0]);
                return 1;
        }
    }

    if (datafile == "" || configfile == "") {
        std::cerr << "Options -c and -i are required." << std::endl << std::endl;
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

        cppdb::session sql(build_connection(config["database"]));

        sql.begin();
        Persistence p(sql, true);

        int64_t count = 0;
        Parser::parseTSV(in, [&sql, &p, &count](Statement st)  {
            p.addStatement(st);
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
