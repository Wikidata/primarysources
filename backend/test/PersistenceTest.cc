//
// Created by schaffert on 4/7/15.
//
#include "gtest.h"

#include <ctime>
#include <fstream>
#include <sstream>
#include <cppdb/frontend.h>
#include <boost/algorithm/string.hpp>

#include "Persistence.h"

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
    time_t rawtime = std::time(NULL);
    std::tm *t = std::gmtime(&rawtime);

    PropertyValue pvs[] = {
            PropertyValue("P123", Value("Hello, World!", "en")),
            PropertyValue("P124", Value("Q321")),
            PropertyValue("P125", Value(42.11, 11.32)),
            PropertyValue("P126", Value(*t, 9)),
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
}

TEST_F(PersistenceTest, UpdateStatement) {
    Statement stmt(-1, "Q1231", PropertyValue("P456", Value("Q789")),
                   Statement::extensions_t(), Statement::extensions_t(), UNAPPROVED);

    Persistence p(sql, true);
    sql.begin();
    int64_t id1 = p.addStatement(stmt);
    p.updateStatement(id1, APPROVED);
    Statement stmt2 = p.getStatement(id1);
    sql.commit();

    ASSERT_EQ(stmt2.getApprovalState(), APPROVED);

}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}