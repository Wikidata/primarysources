//
// Created by schaffert on 4/8/15.
//
#include "gtest.h"

#include "Statement.h"

TEST(ValueTest,Equality) {
    // entity values
    Value entity1("Q123"), entity2("Q123"), entity3("Q124");

    ASSERT_EQ(entity1, entity2);
    ASSERT_NE(entity1, entity3);

    // string values
    Value str1("xxx", "en"), str2("xxx", "en"), str3("xxx", "fr"), str4("yyy", "en");

    ASSERT_EQ(str1, str2);
    ASSERT_NE(str1, str3);
    ASSERT_NE(str1, str4);

    // quantity values
    Value q1(decimal_t(1.0)), q2(decimal_t(1.0)), q3(decimal_t(1.1));

    ASSERT_EQ(q1, q2);
    ASSERT_NE(q1, q3);

    // location values
    Value l1(47.11, 11.47), l2(47.11, 11.47), l3(48.12, 11.47);

    ASSERT_EQ(l1, l2);
    ASSERT_NE(l1, l3);

    // time values
    time_t rawtime;
    std::tm *ptm;
    std::time (&rawtime);
    ptm = std::gmtime(&rawtime);

    Value t1(*ptm, 9), t2(*ptm, 9);

    rawtime++;
    ptm = std::gmtime(&rawtime);
    Value t3(*ptm, 9);

    ASSERT_EQ(t1, t2);
    ASSERT_NE(t1, t3);
}
