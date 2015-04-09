// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolBackend.h"

#include <sstream>
#include <iostream>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>

#include "Parser.h"

#include "Persistence.h"

int64_t SourcesToolBackend::cacheHits = 0;
int64_t SourcesToolBackend::cacheMisses = 0;

std::string build_connection(
        const std::string& db_driver, const std::string& db_name,
        const std::string& db_host, const std::string& db_port,
        const std::string& db_user, const std::string& db_pass
) {
    std::ostringstream out;
    out << db_driver << ":db=" << db_name;
    if (db_host != "") {
        out << ";host=" << db_host;
    }
    if (db_port != "") {
        out << ";port=" << db_port;
    }
    if (db_user != "") {
        out << ";user=" << db_user;
    }
    if (db_pass != "") {
        out << ";pass=" << db_pass;
    }
    return out.str();
}

namespace cppcms {
    template<>
    struct serialization_traits<std::vector<Statement>> {
        ///
        /// Convert string to object
        ///
        static void load(std::string const &serialized_object,std::vector<Statement> &real_object)
        {
            archive a;
            a.str(serialized_object);
            cppcms::details::archive_load_container<std::vector<Statement>>(real_object, a);
        }
        ///
        /// Convert object to string
        ///
        static void save(std::vector<Statement> const &real_object,std::string &serialized_object)
        {
            archive a;
            cppcms::details::archive_save_container<std::vector<Statement>>(real_object, a);
            serialized_object = a.str();
        }
    };
} /* cppcms */

SourcesToolBackend::SourcesToolBackend(const cppcms::json::value& config)
        : cache(cache) {
    connstr = build_connection(
            config["driver"].str(), config["name"].str(), config["host"].str(),
            config["port"].str(), config["user"].str(), config["password"].str());
}


std::vector<Statement> SourcesToolBackend::getStatementsByQID(
        cache_t& cache,
        std::string &qid, bool unapprovedOnly){
    std::vector<Statement> statements;

    // look up in cache and only hit backend in case of a cache miss
    if(!cache.fetch_data(qid, statements)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql);
        statements = p.getStatementsByQID(qid, unapprovedOnly);
        cache.store_data(qid, statements);

        cacheMisses++;
    } else {
        cacheHits++;
    }

    return statements;
}

std::vector<Statement> SourcesToolBackend::getRandomStatements(
        cache_t& cache, int count, bool unapprovedOnly) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    return p.getRandomStatements(count, unapprovedOnly);
}

Statement SourcesToolBackend::getStatementByID(cache_t& cache, int64_t id) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    return p.getStatement(id);
}

void SourcesToolBackend::updateStatement(
        cache_t& cache, int64_t id, ApprovalState state, std::string user) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql, true);
    sql.begin();

    try {
        // make sure statement exists; will throw an exception if not
        Statement st = p.getStatement(id);

        p.updateStatement(id, state);
        p.addUserlog(user, id, state);

        sql.commit();

        // update cache
        cache.rise(st.getQID());
        cache.rise("STATUS");
    } catch (PersistenceException const &e) {
        sql.rollback();

        throw e;
    }

}

std::vector<Statement> SourcesToolBackend::getStatementsByRandomQID(
        cache_t& cache, bool unapprovedOnly) {
    cppdb::session sql(connstr); // released when sql is destroyed

    Persistence p(sql);
    std::string qid = p.getRandomQID(unapprovedOnly);

    std::vector<Statement> statements;

    // look up in cache and only hit backend in case of a cache miss
    if(!cache.fetch_data(qid, statements)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql);
        statements = p.getStatementsByQID(qid, unapprovedOnly);
        cache.store_data(qid, statements);

        cacheMisses++;
    } else {
        cacheHits++;
    }

    return statements;
}

int64_t SourcesToolBackend::importStatements(std::istream &_in, bool gzip, bool dedup) {
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

    int64_t count = 0, first_id = -1, current_id;
    Parser::parseTSV(in, [&sql, &p, &count, %first_id, &current_id](Statement st)  {
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

Status SourcesToolBackend::getStatus(cache_t& cache) {
    Status result;

    if(!cache.fetch_data("STATUS", result)) {
        cppdb::session sql(connstr); // released when sql is destroyed

        Persistence p(sql, true);
        sql.begin();

        result.setStatements(p.countStatements());
        result.setApproved(p.countStatements(APPROVED));
        result.setUnapproved(p.countStatements(UNAPPROVED));
        result.setDuplicate(p.countStatements(DUPLICATE));
        result.setBlacklisted(p.countStatements(BLACKLISTED));
        result.setWrong(p.countStatements(WRONG));
        result.setTopUsers(p.getTopUsers(10));

        sql.commit();

        cache.store_data("STATUS", result, 3600);

        cacheMisses++;
    } else {
        cacheHits++;
    }
    return result;
}
