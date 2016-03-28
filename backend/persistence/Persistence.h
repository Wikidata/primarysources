// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_PERSISTENCE_H_
#define HAVE_PERSISTENCE_H_

#include <cppdb/frontend.h>
#include <cppcms/json.h>
#include <exception>

#include <model/Status.h>
#include "model/Statement.h"

namespace wikidata {
namespace primarysources {

std::string build_connection(const cppcms::json::value& config);

class PersistenceException : public std::exception {
 public:
    explicit PersistenceException(const std::string &message)
            : message(message) { }

    const char *what() const noexcept override;

 private:
    std::string message;
};

/**
* Abstraction class for database persistence. Offers low-level methods for
* querying and updating statements in a given CppDB SQL session.
*
* Implements a certain level of caching using the cppcms caching functionality.
*/
class Persistence {
 public:
    /**
    * Create a new persistence layer using the given database session. If
    * managedTransactions is given and set to true, the persistence assumes
    * that the caller manages transactions and does not do any transaction
    * handling itself for updates (useful for bulk updates).
    */
    Persistence(cppdb::session &sql, bool managedTransactions = false);

    /**
    * Add a new statement to the database. Returns the database ID of the
    * newly added or existing statement.
    */
    int64_t addStatement(const model::Statement& st, bool check_duplicates = false);

    /**
    * Update the database state of the given statement. Currently only takes
    * into account the approval state.
    */
    void updateStatement(const model::Statement& st);

    /**
    * Update the database state of the given statement. Currently only takes
    * into account the approval state.
    */
    void updateStatement(int64_t id, model::ApprovalState state);

    /**
    * Add an entry to the userlog, storing that the given user updated the
    * approval state of the statement with the given id to the given state.
    */
    void addUserlog(
            const std::string& user, int64_t stmtid, model::ApprovalState state);

    /**
    * Retrieve the statement with the given database ID.
    */
    model::Statement getStatement(int64_t id);

    /**
    * Return a list of all statements with the given QID as subject.
    */
    std::vector<model::Statement> getStatementsByQID(
            const std::string &qid, model::ApprovalState state,
            const std::string &dataset = "");

    /**
    * Return a list of count random statements. Selection is up to the backend.
    */
    std::vector<model::Statement> getRandomStatements(
            int count, model::ApprovalState state);

    /**
    * Return all statements.
    */
    std::vector<model::Statement> getAllStatements(
            int offset = 0, int limit = 10,
            model::ApprovalState state = model::ApprovalState::ANY,
            const std::string& dataset = "",
            const std::string& property = "",
            const model::Value* value = nullptr);

    /**
    * Return a list of count random statements concerned with the topic given
    * as string.
    */
//    std::vector<Statement> getRandomStatementsByTopic(
//            const std::string &topic, int count, bool unapprovedOnly);


    /**
    * Retrieve a random QID from the database. If state is not ANY,
    * only return QIDs of entities with at least one statement of the given
    * state.
    *
    * If dataset is set only returns QIDs of entites with at least one
    * statement in this dataset.
    *
    * Throws PersistenceException in case there are no entities (or no
    * unapproved entities).
    */
    std::string getRandomQID(
            model::ApprovalState state = model::ApprovalState::UNAPPROVED,
            const std::string &dataset = "");


    /**
    * Retrieve a random QID from the database for entities having a given
    * topic. If unapprovedOnly is true, only return QIDs of entities with
    * at least one unapproved statement.
    */
//    std::string getRandomQIDByTopic(
//            const std::string& topic, bool unapprovedOnly);

    /**
     * Return the total number of statements in the database.
     */
    int64_t countStatements(const std::string& dataset = "");

    /**
     * Return the number of statements of a given state in the database
     */
    int64_t countStatements(model::ApprovalState state,
                            const std::string& dataset = "");

    /**
     * Return the total number of users in the database
     */
    int32_t countUsers();

    /**
     * Return the top users with respect to the number of approved/rejected
     * statements, ordered by descending number of activities.
     */
    std::vector<model::UserStatus> getTopUsers(int32_t limit=10);

    /**
     * Iterate over all entities in the database, and mark all duplicate
     * statements with approval state 'duplicate' so they no longer appear
     * in the frontend.
     *
     * This is an expensive operation that should be run in the background.
     * Duplicates could also be eliminated during import, but this would
     * considerably slow down import performance. Sweep and mark is a
     * reasonable alternative.
     *
     * Parameters:
     * - start_id: database ID of first statement to consider
     */
    void markDuplicates(int64_t start_id = 0);


    /**
     * Delete statements with the given approval state.
     */
    void deleteStatements(model::ApprovalState state);

    /**
     * Returns the list of all datasets
     */
    std::vector<std::string> getDatasets();


private:
    // reference to the wrapped sql session
    cppdb::session& sql;

    bool managedTransactions;

    long double addSnak(const model::PropertyValue &pv);

    // Return the existing snak id of the given property/value pair.
    // Returns -1 in case the snak does not exist yet.
    int64_t getSnakID(const model::PropertyValue &pv);

    // helper method to distinguish between the case where we check duplicates and
    // the case where we don't
    int64_t getOrAddSnak(const model::PropertyValue &pv, bool check_duplicates);

    // Return the PropertyValue pair for the snak with the given ID.
    // Throws PersistenceException in case the snak with this ID is not found.
    model::PropertyValue getSnak(int64_t snakid);

    // Return the list of log entries for the statement with the given ID.
    // Returns an empty vector in case there are no log entries for this ID.
    std::vector<model::LogEntry> getLogEntries(int64_t stmtid);

    model::Statement buildStatement(int64_t id, const std::string& qid,
                             int64_t snak, const std::string& dataset,
                             int64_t upload, int16_t state);

    friend class PersistenceTest_AddGetSnak_Test;
};

}  // namespace primarysources
}  // namespace wikidata

#endif  // HAVE_PERSISTENCE_H_
