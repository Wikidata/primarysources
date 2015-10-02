// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest.h"

#include <sstream>

#include "Statement.h"
#include "Parser.h"

// helper method: parse statements from a string and return a vector of
// statement objects.
std::vector<Statement> parseString(const std::string& data) {
    std::stringstream buf(data);
    std::vector<Statement> result;

    Parser::parseTSV("freebase", 0, buf, [&result](Statement st) {
        result.push_back(st);
    });

    return result;
}

TEST(ParserTest, ParseEntity) {
    std::vector<Statement> result = parseString("Q123\tP123\tQ321\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue(), Value("Q321"));
}

TEST(ParserTest, ParseString) {
    std::vector<Statement> result = parseString("Q123\tP123\t\"Q321\"\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue(), Value("Q321",""));
}

TEST(ParserTest, ParseLangString) {
    std::vector<Statement> result = parseString("Q123\tP123\ten:\"Q321\"\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue(), Value("Q321","en"));
}

TEST(ParserTest, ParseLocation) {
    std::vector<Statement> result = parseString("Q123\tP123\t@47.11/-10.09\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue(), Value(47.11,-10.09));
}

TEST(ParserTest, ParseTime) {
    std::vector<Statement> result = parseString("Q123\tP123\t+00000001967-01-17T00:00:00Z/11\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue().getTime(), Time(1967, 1, 17));
}

TEST(ParserTest, ParseTimeYear) {
    std::vector<Statement> result = parseString("Q123\tP123\t+00000001967-00-00T00:00:00Z/9\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue().getTime(), Time(1967));
}

TEST(ParserTest, ParseQuantity) {
    std::vector<Statement> result = parseString("Q123\tP123\t+123.21\n");

    ASSERT_EQ(result.size(), 1);
    ASSERT_EQ(result[0].getQID(), "Q123");
    ASSERT_EQ(result[0].getProperty(), "P123");
    ASSERT_EQ(result[0].getValue(), Value(decimal_t("123.21")));
}


TEST(ParserTest, ParseMulti) {
    std::vector<Statement> result = parseString(
            "Q123\tP123\tQ321\n"
            "Q123\tP123\tQ322\n"
            "Q123\tP123\tQ323\n"
            "Q123\tP123\tQ324\n"
    );

    ASSERT_EQ(result.size(), 4);
}
