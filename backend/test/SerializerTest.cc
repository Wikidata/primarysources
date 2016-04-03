// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest/gtest.h"
#include "model/Statement.h"
#include "serializer/SerializerTSV.h"
#include "serializer/SerializerJSON.h"

namespace wikidata {
namespace primarysources {
namespace serializer {

using model::Statement;
using model::NewStatement;
using model::NewPropertyValue;
using model::NewValue;
using model::NewTime;
using model::NewQuantity;

TEST(SerializerTest, TSV) {
    Statement s1 = NewStatement("Q123", NewPropertyValue("P123", NewValue("Q321")));
    std::stringstream out1;
    writeStatementTSV(s1, &out1);
    ASSERT_EQ(out1.str(), "Q123\tP123\tQ321\n");

    Statement s2 = NewStatement("Q123", NewPropertyValue(
            "P123", NewValue("The quick brown fox jumps over the lazy dog", "en")));
    std::stringstream out2;
    writeStatementTSV(s2, &out2);
    ASSERT_EQ(out2.str(), "Q123\tP123\ten:\"The quick brown fox jumps over the lazy dog\"\n");

    Statement s3 = NewStatement("Q123", NewPropertyValue("P123", NewQuantity("123.45")));
    std::stringstream out3;
    writeStatementTSV(s3, &out3);
    ASSERT_EQ(out3.str(), "Q123\tP123\t+123.45\n");

    Statement s4 = NewStatement("Q123", NewPropertyValue("P123", NewTime(1967, 0, 0, 0, 0, 0, 9)));
    std::stringstream out4;
    writeStatementTSV(s4, &out4);
    ASSERT_EQ(out4.str(), "Q123\tP123\t+00000001967-00-00T00:00:00Z/9\n");

    Statement s5 = NewStatement("Q123", NewPropertyValue("P123", NewValue(47.11, -10.09)));
    std::stringstream out5;
    writeStatementTSV(s5, &out5);
    ASSERT_EQ(out5.str(), "Q123\tP123\t@47.11/-10.09\n");
}

}
}  // namespace primarysources
}  // namespace wikidata

