// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_SOURCESTOOL_BACKEND_H_
#define HAVE_SOURCESTOOL_BACKEND_H_

#include <vector>
#include <string>
#include <chrono>
#include <thread>
#include <condition_variable>
#include <mutex>

#include <model/Statement.h>
#include <service/RedisCacheService.h>
#include <service/DashboardService.h>
#include <status/SystemStatus.h>

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

    ~SourcesToolBackend();

    /**
    * Return a statement by ID. Throws PersistenceException if not found.
    */
    model::Statement getStatementByID(cache_t& cache, int64_t id);

    /**
    * Return a list of statements for the given entity ID. If unapprovedOnly
    * is set, return only statements with state "unapproved". If dataset
    * is set return only statements in this dataset
    */
    std::vector<model::Statement> getStatementsByQID(
            cache_t &cache, const std::string &qid,
            model::ApprovalState state = model::ApprovalState::UNAPPROVED,
            const std::string &dataset = "");

    /**
    * Return a list of statements for a randomly selected entity ID. If unapprovedOnly
    * is set, return only statements with state "unapproved".
    */
    std::vector<model::Statement> getStatementsByRandomQID(
            cache_t& cache,
            model::ApprovalState state = model::ApprovalState::UNAPPROVED,
            const std::string& dataset = "");

    /**
    * Return a list of count random statements.
    */
    std::vector<model::Statement> getRandomStatements(
            cache_t &cache, int count,
            model::ApprovalState state = model::ApprovalState::UNAPPROVED);

    /**
    * Return all statements.
    */
    std::vector<model::Statement> getAllStatements(
            cache_t& cache, int offset = 0, int limit = 10,
            model::ApprovalState state = model::ApprovalState::UNAPPROVED,
            const std::string& dataset = "",
            const std::string& property = "",
            const model::Value* value = nullptr);

    /**
    * Update the approval state of the statement with the given ID.
    */
    void updateStatement(
            cache_t& cache, int64_t id, model::ApprovalState state, const std::string& user);

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
    void deleteStatements(cache_t& cache, model::ApprovalState state);


    /**
     * Return status information about the database, e.g. number of approved/
     * unapproved statements, top users, etc.
     */
    model::Status getStatus(cache_t& cache, const std::string& dataset);

    /**
     * Returns the list of all datasets
     */
    std::vector<std::string> getDatasets(cache_t& cache);

    /**
     * Return an activity log for the current database.
     */
    Dashboard::ActivityLog getActivityLog(cache_t& cache);

    /**
     * Return a reference to the status service.
     */
    status::StatusService& StatusService() {
        return status::StatusService::instance(connstr);
    };

    // Clear all cached entities from all caches. Might be expensive.
    void ClearCachedEntities(cache_t& cache);

    // Update Redis entity cache with data from persistence layer.
    void UpdateCachedEntities(cache_t& cache);
 private:

    // Internal method to abstract away from different entity caching backends.
    bool getCachedEntity(cache_t& cache,
                         const std::string& cacheKey,
                         std::vector<model::Statement>* result);

    // Store/update a cached entity
    void storeCachedEntity(cache_t& cache,
                           const std::string& cacheKey,
                           const std::vector<model::Statement>& value);

    // Evict a single cached entity.
    void evictCachedEntity(cache_t& cache, const std::string& cacheKey);

    // Iterate over all statements and update the Redis cache if configured.
    void populateCachedEntities();

    // CppDB uses a connection pool internally, so we just remember the
    // connection string
    std::string connstr;

    // A pointer to a Redis cache service if Redis is configured.
    std::unique_ptr<RedisCacheService> redisSvc;

    std::thread redisUpdater;
    std::condition_variable redisWaiter;
    std::mutex redisMutex;

    bool shutdown = false;
};


}  // namespace primarysources
}  // namespace wikidata
#endif // HAVE_SOURCESTOOL_BACKEND_H_
