// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_PERSISTENCE_H_
#define HAVE_PERSISTENCE_H_

#include <cppdb/frontend.h>
#include <exception>

#include "Statement.h"


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
    int64_t addStatement(const Statement& st, bool check_duplicates = false);

    /**
    * Update the database state of the given statement. Currently only takes
    * into account the approval state.
    */
    void updateStatement(const Statement& st);

    /**
    * Update the database state of the given statement. Currently only takes
    * into account the approval state.
    */
    void updateStatement(int64_t id, ApprovalState state);

    /**
    * Add an entry to the userlog, storing that the given user updated the
    * approval state of the statement with the given id to the given state.
    */
    void addUserlog(
            const std::string& user, int64_t stmtid, ApprovalState state);

    /**
    * Retrieve the statement with the given database ID.
    */
    Statement getStatement(int64_t id);

    /**
    * Return a list of all statements with the given QID as subject.
    */
    std::vector<Statement> getStatementsByQID(
            const std::string& qid, bool unapprovedOnly,
            const std::string& dataset = "");

    /**
    * Return a list of count random statements. Selection is up to the backend.
    */
    std::vector<Statement> getRandomStatements(
            int count, bool unapprovedOnly);

    /**
    * Return a list of count random statements concerned with the topic given
    * as string.
    */
//    std::vector<Statement> getRandomStatementsByTopic(
//            const std::string &topic, int count, bool unapprovedOnly);


    /**
    * Retrieve a random QID from the database. If unapprovedOnly is true,
    * only return QIDs of entities with at least one unapproved statement.
    *
    * If dataset is set only returns QIDs of entites with at least one
    * statement in this dataset.
    *
    * Throws PersistenceException in case there are no entities (or no
    * unapproved entities).
    */
    std::string getRandomQID(bool unapprovedOnly,
                             const std::string& dataset = "");


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
    int64_t countStatements(ApprovalState state, const std::string& dataset = "");

    /**
     * Return the top users with respect to the number of approved/rejected
     * statements, ordered by descending number of activities.
     */
    std::vector<std::pair<std::string,int64_t>> getTopUsers(int32_t limit=10);

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
    void deleteStatements(ApprovalState state);

    /**
     * Returns the list of all datasets
     */
    std::vector<std::string> getDatasets();


private:
    // reference to the wrapped sql session
    cppdb::session& sql;

    bool managedTransactions;

    int64_t addSnak(const PropertyValue &pv);

    // Return the existing snak id of the given property/value pair.
    // Returns -1 in case the snak does not exist yet.
    int64_t getSnakID(const PropertyValue &pv);

    // helper method to distinguish between the case where we check duplicates and
    // the case where we don't
    int64_t getOrAddSnak(const PropertyValue &pv, bool check_duplicates);

    // Return the PropertyValue pair for the snak with the given ID.
    // Throws PersistenceException in case the snak with this ID is not found.
    PropertyValue getSnak(int64_t snakid);

    Statement buildStatement(int64_t id, const std::string& qid,
                             int64_t snak, const std::string& dataset,
                             int64_t upload, int16_t state);

    friend class PersistenceTest_AddGetSnak_Test;
};

#endif  // HAVE_PERSISTENCE_H_
