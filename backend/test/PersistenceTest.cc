// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest/gtest.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <cppdb/frontend.h>
#include <boost/algorithm/string.hpp>

#include "persistence/Persistence.h"

namespace wikidata {
namespace primarysources {

using wikidata::primarysources::model::ApprovalState;
using wikidata::primarysources::model::PropertyValue;
using wikidata::primarysources::model::Statement;
using wikidata::primarysources::model::Value;

using wikidata::primarysources::model::NewQuantity;
using wikidata::primarysources::model::NewTime;
using wikidata::primarysources::model::NewPropertyValue;
using wikidata::primarysources::model::NewValue;


class PersistenceTest : public ::testing::Test {

protected:
    std::string schema_path = "schema.sqlite.sql";

    cppdb::session sql;

    virtual void SetUp() {
        // setup a database and load the schema from the source directory;
        // test therefore needs to have the source directory as current working
        // directory
        sql.open("sqlite3:db=:memory:");

        std::ifstream schemaFile(schema_path);

        ASSERT_FALSE(schemaFile.fail());

        if (!schemaFile.fail()) {
            std::string statement;

            sql.begin();
            while (std::getline(schemaFile, statement, ';')) {
                boost::algorithm::trim(statement);
                if(statement.length() > 0) {
                    sql << statement << cppdb::exec;
                }
            }
            sql.commit();
        } else {
            std::cerr << "could not read schema.sqlite.sql; is the working "
                      << "directory configured properly?" << std::endl;
        }
    }

    virtual void TearDown() {
        sql.close();
    }

};

TEST_F(PersistenceTest, SchemaExists) {
    sql.begin();
    cppdb::result r = sql << "SELECT count(*) FROM statement" << cppdb::row;
    ASSERT_FALSE(r.empty());
    sql.commit();
}

TEST_F(PersistenceTest, AddGetSnak) {
    PropertyValue pvs[] = {
            NewPropertyValue("P123", NewValue("Hello, World!", "en")),
            NewPropertyValue("P124", NewValue("Q321")),
            NewPropertyValue("P125", NewValue(42.11, 11.32)),
            NewPropertyValue("P126", NewTime(1901, 1, 0, 0, 0, 0, 10)),
            NewPropertyValue("P127", NewQuantity("-1234.42")),
    };

    Persistence p(sql, true);
    for (model::PropertyValue& pv : pvs) {
        sql.begin();
        int64_t id1 = p.addSnak(pv);
        int64_t id2 = p.getSnakID(pv);
        PropertyValue pvt = p.getSnak(id1);
        sql.commit();

        ASSERT_EQ(id1, id2);
        ASSERT_EQ(pv.property(), pvt.property());
        ASSERT_EQ(pv.value(), pvt.value());
    }
}

TEST_F(PersistenceTest, AddGetStatement) {
    Statement stmt = NewStatement("Q123", NewPropertyValue("P456", NewValue("Q789")));

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt);
    Statement stmt2 = p.getStatement(id1);
    sql.commit();
    ASSERT_EQ(stmt.qid(), stmt2.qid());
    ASSERT_EQ(stmt.property_value(), stmt2.property_value());

    sql.begin();
    std::vector<Statement> stmts = p.getStatementsByQID("Q123", ApprovalState::ANY);
    sql.commit();
    ASSERT_EQ(stmts.size(), 1);
    ASSERT_EQ(stmts[0].qid(), stmt.qid());
    ASSERT_EQ(stmts[0].property_value(), stmt.property_value());
}

TEST_F(PersistenceTest, UpdateStatement) {
    Statement stmt = NewStatement("Q1231", NewPropertyValue("P456", NewValue("Q789")));

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt);
    p.updateStatement(id1, ApprovalState::APPROVED);
    Statement stmt2 = p.getStatement(id1);
    int64_t approvedCount = p.countStatements(ApprovalState::APPROVED);
    sql.commit();

    ASSERT_EQ(stmt2.approval_state(), ApprovalState::APPROVED);
    ASSERT_EQ(approvedCount, 1);
}


TEST_F(PersistenceTest, MarkDuplicates) {
    Statement stmt = NewStatement("Q1231", NewPropertyValue("P456", NewValue("Q789")));

    Persistence p(sql, true);
    sql.begin();
    for(int i=0; i<10; i++) {
        p.addStatement(stmt);
    }
    sql.commit();

    sql.begin();
    p.markDuplicates();
    sql.commit();

    sql.begin();
    int64_t unapprovedCount = p.countStatements(ApprovalState::UNAPPROVED);
    int64_t duplicateCount = p.countStatements(ApprovalState::DUPLICATE);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 1);
    ASSERT_EQ(duplicateCount, 9);
}


TEST_F(PersistenceTest, RandomQID) {
    Statement stmt1 = NewStatement("Q123", NewPropertyValue("P456", NewValue("Q789")));
    Statement stmt2 = NewStatement("Q124", NewPropertyValue("P456", NewValue("Q789")));
    Statement stmt3 = NewStatement("Q125", NewPropertyValue("P456", NewValue("Q789")));
    Statement stmt4 = NewStatement("Q126", NewPropertyValue("P456", NewValue("Q789")));

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt1);
    int64_t id2 = p.addStatement(stmt2);
    int64_t id3 = p.addStatement(stmt3);
    int64_t id4 = p.addStatement(stmt4);
    sql.commit();

    std::string qid;
    for(int i=0; i<4; i++) {
        sql.begin();
        qid = p.getRandomQID(ApprovalState::UNAPPROVED);
        p.updateStatement(id4 - i, ApprovalState::APPROVED);
        sql.commit();
        ASSERT_NE(qid, "");
    }

    // nothing left, expect exception
    ASSERT_THROW(p.getRandomQID(ApprovalState::UNAPPROVED), PersistenceException);
}


TEST_F(PersistenceTest, AllStatements) {
    Statement stmt1 = NewStatement(
            -1, "Q123", NewPropertyValue("P234", NewValue("Q789")),
            {}, {}, "foo", 0, ApprovalState::UNAPPROVED);
    Statement stmt2 = NewStatement(
            -1, "Q124", NewPropertyValue("P234", NewValue("Q789")),
            {}, {}, "foo", 0, ApprovalState::UNAPPROVED);
    Statement stmt3 = NewStatement(
            -1, "Q125", NewPropertyValue("P457", NewValue("Q789")),
            {}, {}, "foo", 0, ApprovalState::UNAPPROVED);
    Statement stmt4 = NewStatement(
            -1, "Q127", NewPropertyValue("P234", NewValue("Q789")),
            {}, {}, "bar", 0, ApprovalState::UNAPPROVED);
    Statement stmt5 = NewStatement(
            -1, "Q124", NewPropertyValue("P234", NewValue("Q790")),
            {}, {}, "foo", 0, ApprovalState::UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    p.addStatement(stmt1);
    p.addStatement(stmt2);
    p.addStatement(stmt3);
    p.addStatement(stmt4);
    p.addStatement(stmt5);
    sql.commit();

    sql.begin();
    Value value = NewValue("Q789");
    std::vector<Statement> statements = p.getAllStatements(0, 10, ApprovalState::ANY, "foo", "P234", &value);
    sql.commit();

    ASSERT_EQ(statements.size(), 2);
}


TEST_F(PersistenceTest, DeleteStatements) {
    Persistence p(sql, true);
    sql.begin();
    for(int i=0; i<10; i++) {
        Statement stmt = NewStatement("Q1231", NewPropertyValue("P456", NewValue("Q789-" + i)));
        p.addStatement(stmt);
    }
    sql.commit();

    sql.begin();

    // SQLLite database IDs start at 1
    for(int64_t i=1; i<6; i++) {
        p.updateStatement(i, ApprovalState::APPROVED);
    }
    sql.commit();

    sql.begin();
    int64_t unapprovedCount = p.countStatements(ApprovalState::UNAPPROVED);
    int64_t approvedCount = p.countStatements(ApprovalState::APPROVED);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 5);
    ASSERT_EQ(approvedCount, 5);

    sql.begin();
    p.deleteStatements(ApprovalState::UNAPPROVED);
    sql.commit();

    sql.begin();
    unapprovedCount = p.countStatements(ApprovalState::UNAPPROVED);
    approvedCount = p.countStatements(ApprovalState::APPROVED);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 0);
    ASSERT_EQ(approvedCount, 5);
}

TEST_F(PersistenceTest, Datasets) {
    Persistence p(sql, true);
    sql.begin();
    for (int i = 0; i < 10; i++) {
        Statement stmt = NewStatement(
                -1, "Q1231", NewPropertyValue("P456", NewValue("Q789-" + i)),
                {}, {}, "dataset-" + std::to_string(i), i, ApprovalState::UNAPPROVED);

        p.addStatement(stmt);
    }
    sql.commit();

    sql.begin();
    std::vector<std::string> datasets = p.getDatasets();
    sql.commit();

    ASSERT_EQ(datasets.size(), 10);

    sql.begin();
    int64_t count_exist = p.countStatements("dataset-0");
    int64_t count_noexist = p.countStatements("dataset-nonexistant");
    int64_t count_all = p.countStatements();
    sql.commit();

    ASSERT_EQ(count_exist, 1);
    ASSERT_EQ(count_noexist, 0);
    ASSERT_EQ(count_all, 10);
}

TEST_F(PersistenceTest, Activities) {
    Statement stmt1 = NewStatement(
            -1, "Q123", NewPropertyValue("P234", NewValue("Q789")),
            {}, {}, "foo", 0, ApprovalState::UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    p.addStatement(stmt1);
    sql.commit();

    sql.begin();
    std::vector<Statement> stmts = p.getRandomStatements(1, ApprovalState::ANY);
    sql.commit();

    ASSERT_EQ(stmts.size(), 1);

    int64_t id = stmts[0].id();

    sql.begin();
    p.updateStatement(id, ApprovalState::WRONG);
    p.addUserlog("foouser", id, ApprovalState::WRONG);

    p.updateStatement(id, ApprovalState::APPROVED);
    p.addUserlog("baruser", id, ApprovalState::APPROVED);
    sql.commit();

    sql.begin();
    Statement stmt2 = p.getStatement(id);
    sql.commit();

    ASSERT_EQ(stmt2.activities().size(), 2);
}
}  // namespace primarysources
}  // namespace wikidata
