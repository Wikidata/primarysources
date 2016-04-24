// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <ctime>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <vector>

#include <gflags/gflags.h>
#include <glog/logging.h>

#include <cppcms/json.h>
#include <util/TimeLogger.h>
#include <persistence/Persistence.h>
#include <service/RedisCacheService.h>

DEFINE_string(c, "", "backend configuration file to read database and Redisconfiguration");
DEFINE_string(mode, "update", "cache update mode (update or clear)");

using wikidata::primarysources::TimeLogger;
using wikidata::primarysources::Persistence;
using wikidata::primarysources::RedisCacheService;

using wikidata::primarysources::model::ApprovalState ;
using wikidata::primarysources::model::Statement;
using wikidata::primarysources::model::Statements;

namespace {
// Create cache key for an entity and dataset; the cache key is used to cache
// all statements of the given dataset having the entity as subject. If dataset
// is "", the cache key refers to all statements and the dataset name "all" will
// be used.
std::string createCacheKey(const std::string &qid, ApprovalState state, const std::string &dataset) {
    if (dataset == "") {
        return qid + "-" + std::to_string(state);
    } else {
        return qid + "-" + dataset + "-" + std::to_string(state);
    }
}
}  // namespace

int main(int argc, char **argv) {
    google::InitGoogleLogging(argv[0]);
    gflags::ParseCommandLineFlags(&argc, &argv, true);


    if (FLAGS_c == "") {
        std::cerr << "Option -c is required." << std::endl << std::endl;
        return 1;
    }

    // read configuration
    cppcms::json::value config;
    std::ifstream cfgfile(FLAGS_c);
    cfgfile >> config;

    RedisCacheService redis(config["redis"]["host"].str(),
                            config["redis"]["port"].number());

    if (FLAGS_mode == "clear") {
        TimeLogger timer("Clearing cached Redis entries");
        redis.Clear();
        return 0;
    }

    if (FLAGS_mode != "update") {
        LOG(ERROR) << "unknown mode: " << FLAGS_mode;
        return 1;
    }

    try {
        cppdb::session sql(
                wikidata::primarysources::build_connection(config["database"]));

        sql.begin();
        wikidata::primarysources::Persistence p(sql, true);

        LOG(INFO) << "Start refreshing all cached Redis entries ...";

        auto datasets = p.getDatasets();
        datasets.insert(datasets.begin(), "");

        long count = 0;
        for (const auto& dataset : datasets) {
            TimeLogger timer("Refreshing cached Redis entries for " +
                             (dataset == "" ? "all datasets" : "dataset " + dataset));

            std::cout << "Updating Redis entries for " <<
                    (dataset == "" ? "all datasets" : "dataset " + dataset) << std::endl;

            Statements stmts;
            std::string qid = "";

            p.getAllStatements([&redis, &qid, &dataset, &stmts, &count](const Statement& s) {
                if (qid != s.qid() && qid != "") {
                    // store current batch of statements and clear it
                    redis.Add(createCacheKey(qid, ApprovalState::UNAPPROVED, dataset), stmts);
                    stmts.clear_statements();
                    count++;

                    if (count % 1000 == 0) {
                        std::cout << "Updated entries: " << count << std::endl;
                    }
                }
                *stmts.add_statements() = s;
                qid = s.qid();
            }, ApprovalState::UNAPPROVED, dataset);

            if (stmts.statements_size() > 0) {
                redis.Add(createCacheKey(qid, ApprovalState::UNAPPROVED, dataset), stmts);
            }
        }

        LOG(INFO) << "Finished refreshing all cached Redis entries.";

        sql.commit();

        return 0;
    } catch (std::exception const &e) {
        LOG(ERROR) << e.what();

        return 1;
    }
}
