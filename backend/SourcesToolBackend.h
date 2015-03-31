// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_SOURCESTOOL_BACKEND_H_
#define HAVE_SOURCESTOOL_BACKEND_H_

#include "Statement.h"
#include "Status.h"

#include <vector>
#include <map>
#include <string>

#include <cppcms/cache_interface.h>
#include <cppcms/json.h>
#include <cppdb/frontend.h>

std::string build_connection(
        const std::string& db_driver, const std::string& db_name,
        const std::string& db_host, const std::string& db_port,
        const std::string& db_user, const std::string& db_pass
);

/**
* Database backend for accessing statements and entities. Manages the
* SQL connection details and wraps the Persistence implementation.
*/
class SourcesToolBackend {
public:
    typedef cppcms::cache_interface cache_t;

    SourcesToolBackend(const cppcms::json::value& config);


    /**
    * Return a statement by ID. Throws PersistenceException if not found.
    */
    Statement getStatementByID(cache_t& cache, int64_t id);

    /**
    * Return a list of statements for the given entity ID. If unapprovedOnly
    * is set, return only statements with state "unapproved".
    */
    std::vector<Statement> getStatementsByQID(cache_t& cache, std::string &qid, bool unapprovedOnly);

    /**
    * Return a list of statements for a randomly selected entity ID. If unapprovedOnly
    * is set, return only statements with state "unapproved".
    */
    std::vector<Statement> getStatementsByRandomQID(cache_t& cache,bool unapprovedOnly);

    /**
    * Return a list of count random statements.
    */
    std::vector<Statement> getRandomStatements(cache_t& cache, int count, bool unapprovedOnly);

    /**
    * Update the approval state of the statement with the given ID.
    */
    void updateStatement(cache_t& cache, int64_t id, ApprovalState state, std::string user);

    /**
     * Return status information about the database, e.g. number of approved/
     * unapproved statements, top users, etc.
     */
    Status getStatus();

    /**
    * Import a (possibly large) list of statements in Wikidata TSV format
    * from the given input stream. The stream may optionally contain gzip'ed
    * data. Returns the number of statements imported.
    */
    int64_t importStatements(std::istream& in, bool gzip);
private:

    // CppDB uses a connection pool internally, so we just remember the
    // connection string
    std::string connstr;

    cppcms::cache_interface& cache;
};


#endif // HAVE_SOURCESTOOL_BACKEND_H_