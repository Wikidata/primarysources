// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_SOURCESTOOL_BACKEND_H_
#define HAVE_SOURCESTOOL_BACKEND_H_

#include "model/Statement.h"
#include "Status.h"
#include "Dashboard.h"

#include <vector>
#include <string>

#include <cppcms/cache_interface.h>
#include <cppcms/json.h>
#include <cppdb/frontend.h>

namespace wikidata {
namespace primarysources {

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
    * is set, return only statements with state "unapproved". If dataset
    * is set return only statements in this dataset
    */
    std::vector<Statement> getStatementsByQID(cache_t &cache, const std::string &qid,
                                              ApprovalState state, const std::string &dataset = "");

    /**
    * Return a list of statements for a randomly selected entity ID. If unapprovedOnly
    * is set, return only statements with state "unapproved".
    */
    std::vector<Statement> getStatementsByRandomQID(cache_t& cache,
                                                    ApprovalState state = UNAPPROVED,
                                                    const std::string& dataset = "");

    /**
    * Return a list of count random statements.
    */
    std::vector<Statement> getRandomStatements(cache_t &cache, int count,
                                               ApprovalState state = UNAPPROVED);

    /**
    * Return all statements.
    */
    std::vector<Statement> getAllStatements(cache_t& cache, int offset = 0, int limit = 10,
                                            ApprovalState state = UNAPPROVED,
                                            const std::string& dataset = "",
                                            const std::string& property = "",
                                            const Value* value = nullptr);

    /**
    * Update the approval state of the statement with the given ID.
    */
    void updateStatement(cache_t& cache, int64_t id, ApprovalState state, const std::string& user);

    /**
    * Import a (possibly large) list of statements in Wikidata TSV format
    * from the given input stream. The stream may optionally contain gzip'ed
    * data. When the option dedup is set to true, will run a deduplication
    * sweep at the end of importing. Returns the number of statements imported.
    */
    int64_t importStatements(cache_t& cache, std::istream& in,
                             const std::string& dataset, bool gzip, bool dedup=true);


    /**
     * Delete statements with the given approval state.
     */
    void deleteStatements(cache_t& cache, ApprovalState state);


    /**
     * Return status information about the database, e.g. number of approved/
     * unapproved statements, top users, etc.
     */
    Status getStatus(cache_t& cache);

    /**
     * Returns the list of all datasets
     */
    std::vector<std::string> getDatasets(cache_t& cache);

    /**
     * Return an activity log for the current database.
     */
    Dashboard::ActivityLog getActivityLog(cache_t& cache);

private:

    // CppDB uses a connection pool internally, so we just remember the
    // connection string
    std::string connstr;
};


}  // namespace primarysources
}  // namespace wikidata
#endif // HAVE_SOURCESTOOL_BACKEND_H_
