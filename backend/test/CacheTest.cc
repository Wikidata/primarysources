// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest.h"
#include "model/Statement.h"
#include "service/StatementCaching.h"

namespace wikidata {
namespace primarysources {
namespace model {

TEST(CacheTest, Value) {
    std::string archive;

    Value entity1 = NewValue("Q123");
    cppcms::serialization_traits<Value>::save(entity1, archive);

    Value entity2;
    cppcms::serialization_traits<Value>::load(archive, entity2);

    ASSERT_EQ(entity1, entity2);
}

TEST(CacheTest, PropertyValueTest) {
    std::string archive;

    PropertyValue pv1 = NewPropertyValue("P123", NewValue("Q123"));
    cppcms::serialization_traits<PropertyValue>::save(pv1, archive);

    PropertyValue pv2;
    cppcms::serialization_traits<PropertyValue>::load(archive, pv2);

    ASSERT_EQ(pv1, pv2);
}


TEST(CacheTest, StatementTest) {
    std::string archive;

    Statement s1 = NewStatement("Q123", NewPropertyValue("P123", NewValue("Q321")));
    cppcms::serialization_traits<Statement>::save(s1, archive);

    Statement s2;
    cppcms::serialization_traits<Statement>::load(archive, s2);

    ASSERT_EQ(s1, s2);
}


}  // namespace model
}  // namespace primarysources
}  // namespace wikidata
