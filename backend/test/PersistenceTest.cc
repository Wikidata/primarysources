// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <cppdb/frontend.h>
#include <boost/algorithm/string.hpp>

#include "persistence/Persistence.h"

namespace wikidata {
namespace primarysources {

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
            PropertyValue("P123", Value("Hello, World!", "en")),
            PropertyValue("P124", Value("Q321")),
            PropertyValue("P125", Value(42.11, 11.32)),
            PropertyValue("P126", Value(Time(1901, 1))),
            PropertyValue("P127", Value(Quantity("-1234.42"))),
    };

    Persistence p(sql, true);
    for (PropertyValue& pv : pvs) {
        sql.begin();
        int64_t id1 = p.addSnak(pv);
        int64_t id2 = p.getSnakID(pv);
        PropertyValue pvt = p.getSnak(id1);
        sql.commit();

        ASSERT_EQ(id1, id2);
        ASSERT_EQ(pv.getProperty(), pvt.getProperty());
        ASSERT_EQ(pv.getValue(), pvt.getValue());
    }
}

TEST_F(PersistenceTest, AddGetStatement) {
    Statement stmt(-1, "Q123", PropertyValue("P456", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt);
    Statement stmt2 = p.getStatement(id1);
    sql.commit();
    ASSERT_EQ(stmt.getQID(), stmt2.getQID());
    ASSERT_EQ(stmt.getProperty(), stmt2.getProperty());
    ASSERT_EQ(stmt.getValue().getString(), stmt2.getValue().getString());

    sql.begin();
    std::vector<Statement> stmts = p.getStatementsByQID("Q123", ANY);
    sql.commit();
    ASSERT_EQ(stmts.size(), 1);
    ASSERT_EQ(stmts[0].getQID(), stmt.getQID());
    ASSERT_EQ(stmts[0].getProperty(), stmt.getProperty());
    ASSERT_EQ(stmts[0].getValue().getString(), stmt.getValue().getString());

}

TEST_F(PersistenceTest, UpdateStatement) {
    Statement stmt(-1, "Q1231", PropertyValue("P456", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt);
    p.updateStatement(id1, APPROVED);
    Statement stmt2 = p.getStatement(id1);
    int64_t approvedCount = p.countStatements(APPROVED);
    sql.commit();

    ASSERT_EQ(stmt2.getApprovalState(), APPROVED);
    ASSERT_EQ(approvedCount, 1);
}


TEST_F(PersistenceTest, MarkDuplicates) {
    Statement stmt(-1, "Q1231", PropertyValue("P456", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

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
    int64_t unapprovedCount = p.countStatements(UNAPPROVED);
    int64_t duplicateCount = p.countStatements(DUPLICATE);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 1);
    ASSERT_EQ(duplicateCount, 9);
}


TEST_F(PersistenceTest, RandomQID) {
    Statement stmt1(-1, "Q123", PropertyValue("P456", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);
    Statement stmt2(-1, "Q124", PropertyValue("P456", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);
    Statement stmt3(-1, "Q125", PropertyValue("P456", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);
    Statement stmt4(-1, "Q126", PropertyValue("P456", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

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
        qid = p.getRandomQID(UNAPPROVED);
        p.updateStatement(id4 - i, APPROVED);
        sql.commit();
        ASSERT_NE(qid, "");
    }

    // nothing left, expect exception
    ASSERT_THROW(p.getRandomQID(UNAPPROVED), PersistenceException);
}


TEST_F(PersistenceTest, AllStatements) {
    Statement stmt1(-1, "Q123", PropertyValue("P234", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(),
                   "foo", 0, UNAPPROVED);
    Statement stmt2(-1, "Q124", PropertyValue("P234", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(),
                    "foo", 0, UNAPPROVED);
    Statement stmt3(-1, "Q125", PropertyValue("P457", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(),
                    "foo", 0, UNAPPROVED);
    Statement stmt4(-1, "Q127", PropertyValue("P234", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(),
                    "bar", 0, UNAPPROVED);
    Statement stmt5(-1, "Q124", PropertyValue("P234", Value("Q790")),
                    Statement::extensions_t(), Statement::extensions_t(),
                    "foo", 0, UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    p.addStatement(stmt1);
    p.addStatement(stmt2);
    p.addStatement(stmt3);
    p.addStatement(stmt4);
    p.addStatement(stmt5);
    sql.commit();

    sql.begin();
    std::unique_ptr<Value> value(new Value("Q789"));
    std::vector<Statement> statements = p.getAllStatements(0, 10, ANY, "foo", "P234", value.get());
    sql.commit();

    ASSERT_EQ(statements.size(), 2);
}


TEST_F(PersistenceTest, DeleteStatements) {
    Persistence p(sql, true);
    sql.begin();
    for(int i=0; i<10; i++) {
        Statement stmt(-1, "Q1231", PropertyValue("P456", Value("Q789-" + i)),
                       Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

        p.addStatement(stmt);
    }
    sql.commit();

    sql.begin();

    // SQLLite database IDs start at 1
    for(int64_t i=1; i<6; i++) {
        p.updateStatement(i, APPROVED);
    }
    sql.commit();

    sql.begin();
    int64_t unapprovedCount = p.countStatements(UNAPPROVED);
    int64_t approvedCount = p.countStatements(APPROVED);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 5);
    ASSERT_EQ(approvedCount, 5);

    sql.begin();
    p.deleteStatements(UNAPPROVED);
    sql.commit();

    sql.begin();
    unapprovedCount = p.countStatements(UNAPPROVED);
    approvedCount = p.countStatements(APPROVED);
    sql.commit();

    ASSERT_EQ(unapprovedCount, 0);
    ASSERT_EQ(approvedCount, 5);
}

TEST_F(PersistenceTest, Datasets) {
    Persistence p(sql, true);
    sql.begin();
    for (int i = 0; i < 10; i++) {
        Statement stmt(-1, "Q1231", PropertyValue("P456", Value("Q789-" + i)),
                       Statement::extensions_t(), Statement::extensions_t(),
                       "dataset-" + std::to_string(i), i, UNAPPROVED);

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
    Statement stmt1(-1, "Q123", PropertyValue("P234", Value("Q789")),
                    Statement::extensions_t(), Statement::extensions_t(),
                    "foo", 0, UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    p.addStatement(stmt1);
    sql.commit();

    sql.begin();
    std::vector<Statement> stmts = p.getRandomStatements(1, ANY);
    sql.commit();

    ASSERT_EQ(stmts.size(), 1);

    int64_t id = stmts[0].getID();

    sql.begin();
    p.updateStatement(id, WRONG);
    p.addUserlog("foouser", id, WRONG);

    p.updateStatement(id, APPROVED);
    p.addUserlog("baruser", id, APPROVED);
    sql.commit();

    sql.begin();
    Statement stmt2 = p.getStatement(id);
    sql.commit();

    ASSERT_EQ(stmt2.getActivities().size(), 2);
}
}  // namespace primarysources
}  // namespace wikidata
