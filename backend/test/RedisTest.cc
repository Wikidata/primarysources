// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "gtest/gtest.h"
#include "model/Statement.h"
#include "service/RedisCacheService.h"

namespace wikidata {
namespace primarysources {

class RedisTest : public ::testing::Test {
 public:
    RedisTest() : cacheService("localhost", 6379) {};

 protected:
    RedisCacheService cacheService;
};

TEST_F(RedisTest, RoundtripString) {
    std::string key = "foo";
    std::string value = "bar";

    cacheService.Add(key, value);

    std::string result;
    EXPECT_TRUE(cacheService.Get(key, &result));

    EXPECT_EQ(result, value);

    cacheService.Evict(key);
}


TEST_F(RedisTest, RoundtripMessage) {
    model::Statements stmts;
    *stmts.add_statements() = model::NewStatement("Q123", model::NewPropertyValue("P123", model::NewValue("Q321")));
    *stmts.add_statements() = model::NewStatement("Q124", model::NewPropertyValue("P123", model::NewValue("Q321")));


    std::string key = "foo";

    cacheService.Add(key, stmts);

    model::Statements result;
    EXPECT_TRUE(cacheService.Get(key, &result));

    for (int i=0; i<stmts.statements_size(); i++) {
        EXPECT_EQ(result.statements(i), stmts.statements(i));
    }

    cacheService.Evict(key);
}

TEST_F(RedisTest, NotFound) {
    std::string key = "foo";
    std::string result;
    EXPECT_FALSE(cacheService.Get(key, &result));
}

TEST_F(RedisTest, Clear) {
    std::string key = "foo";
    std::string value = "bar";

    cacheService.Add(key, value);

    cacheService.Clear();

    std::string result;
    EXPECT_FALSE(cacheService.Get(key, &result));
}

}  // namespace primarysources
}  // namespace wikidata