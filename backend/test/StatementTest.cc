// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest/gtest.h"
#include "model/Statement.h"

namespace wikidata {
namespace primarysources {
namespace model {

TEST(ValueTest, Equality) {
    // entity values
    Value entity1 = NewValue("Q123");
    Value entity2 = NewValue("Q123");
    Value entity3 = NewValue("Q124");

    ASSERT_EQ(entity1, entity2);
    ASSERT_NE(entity1, entity3);

    // string values
    Value str1 = NewValue("xxx", "en");
    Value str2 = NewValue("xxx", "en");
    Value str3 = NewValue("xxx", "fr");
    Value str4 = NewValue("yyy", "en");

    ASSERT_EQ(str1, str2);
    ASSERT_NE(str1, str3);
    ASSERT_NE(str1, str4);

    // quantity values
    Value q1 = NewQuantity(1.0);
    Value q2 = NewQuantity(1.0);
    Value q3 = NewQuantity(1.1);

    ASSERT_EQ(q1, q2);
    ASSERT_NE(q1, q3);

    // location values
    Value l1 = NewValue(47.11, 11.47);
    Value l2 = NewValue(47.11, 11.47);
    Value l3 = NewValue(48.12, 11.47);

    ASSERT_EQ(l1, l2);
    ASSERT_NE(l1, l3);

    // time values
    Value t1 = NewTime(1923, 1, 1, 0, 0, 0, 11);
    Value t2 = NewTime(1923, 1, 2, 0, 0, 0, 11);
    Value t3 = t1;
    t3.mutable_time()->set_precision(9);
    Value t4 = t3;

    ASSERT_NE(t1, t2);
    ASSERT_NE(t1, t3);
    ASSERT_NE(t1, t4);
    ASSERT_EQ(t3, t4);
}

TEST(TimeTest, toWikidataString) {
    Value t1 = NewTime(1917, 01, 01, 0, 0, 0, 11);
    Value t2 = NewTime(1917, 0, 0, 0, 0, 0, 9);

    ASSERT_EQ(toWikidataString(t1.time()), "+00000001917-01-01T00:00:00Z/11");
    ASSERT_EQ(toWikidataString(t2.time()), "+00000001917-00-00T00:00:00Z/9");
}

TEST(TimeTest, toSQLString) {
    Value t1 = NewTime(1917, 01, 01, 0, 0, 0, 11);
    Value t2 = NewTime(1917, 0, 0, 0, 0, 0, 9);

    ASSERT_EQ(toSQLString(t1.time()), "1917-1-1 0:0:0");
    ASSERT_EQ(toSQLString(t2.time()), "1917-0-0 0:0:0");
}

TEST(ValueTest, getQuantityAsString) {
    Value q1 = NewQuantity("100000000000000000000");
    Value q2 = NewQuantity("-0.000001");

    ASSERT_EQ(q1.quantity().decimal(), "+100000000000000000000");
    ASSERT_EQ(q2.quantity().decimal(), "-0.000001");
}

TEST(PropertyValueTest, Equality) {
    PropertyValue pv1 = NewPropertyValue("P123", NewValue("Q123"));
    PropertyValue pv2 = NewPropertyValue("P123", NewValue("Q123"));
    PropertyValue pv3 = NewPropertyValue("P123", NewValue("Q123", "en"));
    PropertyValue pv4 = NewPropertyValue("P234", NewValue("Q123"));

    ASSERT_EQ(pv1, pv2);
    ASSERT_NE(pv1, pv3);
    ASSERT_NE(pv1, pv4);
}

TEST(StatementTest, Equality) {
    Statement s1 = NewStatement("Q123", NewPropertyValue("P123", NewValue("Q321")));
    Statement s2 = NewStatement("Q123", NewPropertyValue("P123", NewValue("Q321")));
    Statement s3 = NewStatement("Q124", NewPropertyValue("P123", NewValue("Q321")));
    Statement s4 = NewStatement("Q123", NewPropertyValue("P124", NewValue("Q321")));
    Statement s5 = NewStatement("Q123", NewPropertyValue("P123", NewValue("Q421")));

    ASSERT_EQ(s1, s2);
    ASSERT_NE(s1, s3);
    ASSERT_NE(s1, s4);
    ASSERT_NE(s1, s5);
}

}  // namespace model
}  // namespace primarysources
}  // namespace wikidata
