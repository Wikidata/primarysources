// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolBackend.h"

#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <chrono>

#include <model/Statement.h>
#include <parser/Parser.h>
#include <persistence/Persistence.h>
#include <status/SystemStatus.h>
#include <service/StatementCaching.h>

using wikidata::primarysources::model::ApprovalState;
using wikidata::primarysources::model::Statement;

namespace wikidata {
namespace primarysources {
namespace {
// Create cache key for an entity and dataset; the cache key is used to cache
// all statements of the given dataset having the entity as subject. If dataset
// is "", the cache key refers to all statements and the dataset name "all" will
// be used.
std::string createCacheKey(const std::string &qid, ApprovalState state, const std::string &dataset) {
    if (dataset == "") {
        return "ALL-" + qid + "-" + model::stateToString(state);
    } else {
        return qid + "-" + dataset + "-" + model::stateToString(state);
    }
}
}  // namespace

SourcesToolBackend::SourcesToolBackend(const cppcms::json::value& config)
    : connstr(build_connection(config)) {
    // trigger initialisation of the singleton
    StatusService();
}


std::vector<Statement> SourcesToolBackend::getStatementsByQID(
        cache_t &cache, const std::string &qid,
        ApprovalState state, const std::string &dataset){
    std::vector<Statement> statements;
    std::string cacheKey = createCacheKey(qid,state,dataset);

    // look up in cache and only hit backend in case of a cache miss
    if(!cache.fetch_data(cacheKey, statements)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql);
        statements = p.getStatementsByQID(qid, state, dataset);
        cache.store_data(cacheKey, statements, 3600);

        StatusService().AddCacheMiss();
    } else {
        StatusService().AddCacheHit();
    }

    return statements;
}

std::vector<Statement> SourcesToolBackend::getRandomStatements(
        cache_t &cache, int count, ApprovalState state) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    return p.getRandomStatements(count, state);
}

std::vector<Statement> SourcesToolBackend::getAllStatements(
        cache_t& cache, int offset, int limit,
        ApprovalState state,
        const std::string& dataset,
        const std::string& property,
        const model::Value* value) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    return p.getAllStatements(offset, limit, state, dataset, property, value);
}

Statement SourcesToolBackend::getStatementByID(cache_t& cache, int64_t id) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    return p.getStatement(id);
}

void SourcesToolBackend::updateStatement(
        cache_t& cache, int64_t id, ApprovalState state, const std::string& user) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql, true);
    sql.begin();

    try {
        // make sure statement exists; will throw an exception if not
        Statement st = p.getStatement(id);

        p.updateStatement(id, state);
        if (state != ApprovalState::DUPLICATE && state != ApprovalState::BLACKLISTED) {
            p.addUserlog(user, id, state);
        }

        sql.commit();

        // update cache
        for (ApprovalState state : { ApprovalState::APPROVED, ApprovalState::UNAPPROVED,
                                     ApprovalState::WRONG, ApprovalState::DUPLICATE,
                                     ApprovalState::BLACKLISTED }) {
            cache.rise(createCacheKey(st.qid(), state, st.dataset()));
            cache.rise(createCacheKey(st.qid(), state, ""));
        }
        cache.rise("ACTIVITIES");
        StatusService().SetDirty();
    } catch (PersistenceException const &e) {
        sql.rollback();

        throw e;
    }

}

std::vector<Statement> SourcesToolBackend::getStatementsByRandomQID(
        cache_t& cache, ApprovalState state, const std::string& dataset) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    std::string qid = p.getRandomQID(state, dataset);

    std::vector<Statement> statements;
    std::string cacheKey = createCacheKey(qid, state, dataset);

    // look up in cache and only hit backend in case of a cache miss
    if(!cache.fetch_data(cacheKey, statements)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql);
        statements = p.getStatementsByQID(qid, state, dataset);
        cache.store_data(cacheKey, statements, 3600);

        StatusService().AddCacheMiss();
    } else {
        StatusService().AddCacheHit();
    }

    return statements;
}

int64_t SourcesToolBackend::importStatements(cache_t& cache, std::istream &_in,
                                             const std::string& dataset, bool gzip, bool dedup) {
    // prepare GZIP input stream
    boost::iostreams::filtering_istreambuf zin;
    if (gzip) {
        zin.push(boost::iostreams::gzip_decompressor());
    }
    zin.push(_in);
    std::istream in(&zin);

    cppdb::session sql(connstr); // released when sql is destroyed

    sql.begin();
    Persistence p(sql, true);

    // get timestamp and use it as upload id
    int64_t upload = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();

    int64_t count = 0, first_id = -1, current_id;
    parser::parseTSV(dataset, upload, in, [&sql, &p, &count, &first_id, &current_id](Statement st)  {
        current_id = p.addStatement(st);

        // remember the ID of the first statement we add for deduplication
        if (first_id == -1) {
            first_id = current_id;
        }

        count++;

        // batch commit
        if(count % 100000 == 0) {
            sql.commit();
            sql.begin();
        }
    });
    sql.commit();

    if (dedup) {
        sql.begin();
        p.markDuplicates(first_id);
        sql.commit();
    }

    cache.clear();

    return count;
}

void SourcesToolBackend::deleteStatements(cache_t& cache, ApprovalState state) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    p.deleteStatements(state);

    cache.clear();
}

model::Status SourcesToolBackend::getStatus(cache_t& cache, const std::string& dataset) {
    return StatusService().Status(dataset);
}


Dashboard::ActivityLog SourcesToolBackend::getActivityLog(cache_t& cache) {

    Dashboard::ActivityLog result;

    if(!cache.fetch_data("ACTIVITIES", result)) {
        cppdb::session sql(connstr); // released when sql is destroyed
        Dashboard::Dashboard dashboard(sql);
        result = dashboard.getActivityLog(7, 12, 10);

        cache.store_data("ACTIVITIES", result, 3600);

        StatusService().AddCacheMiss();
    } else {
        StatusService().AddCacheHit();
    }

    return result;
}


std::vector<std::string> SourcesToolBackend::getDatasets(cache_t& cache) {
    std::vector<std::string> datasets;

    // look up in cache and only hit backend in case of a cache miss
    if(!cache.fetch_data("datasets", datasets)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql);
        datasets = p.getDatasets();
        cache.store_data("datasets", datasets, 3600);

        StatusService().AddCacheMiss();
    } else {
        StatusService().AddCacheHit();
    }

    return datasets;
}


}  // namespace primarysources
}  // namespace wikidata

