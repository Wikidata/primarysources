// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest/gtest.h"
#include "util/Snowflake.h"

namespace wikidata {
namespace primarysources {

TEST(Snowflake, TestWithoutId) {
    int64_t id1 = Snowflake();
    int64_t id2 = Snowflake();

    ASSERT_GT(id1, 0);
    ASSERT_GT(id2, 0);
    ASSERT_NE(id1, id2);
}

TEST(Snowflake, TestWithId) {
    int64_t id1 = Snowflake(1234);
    int64_t id2 = Snowflake(1234);

    ASSERT_GT(id1, 0);
    ASSERT_GT(id2, 0);
    ASSERT_NE(id1, id2);
}

}  // namespace primarysources
}  // namespace wikidata
