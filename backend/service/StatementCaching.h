//
// Created by wastl on 25.03.16.
//

#ifndef PRIMARYSOURCES_STATEMENTCACHING_H
#define PRIMARYSOURCES_STATEMENTCACHING_H

#include <cppcms/cache_interface.h>

#include <model/statement.pb.h>

namespace cppcms {

using ::wikidata::primarysources::model::PropertyValue;
using ::wikidata::primarysources::model::Statement;
using ::wikidata::primarysources::model::Value;

template<>
struct serialization_traits<Value> {

    static void load(const std::string& serialized_object, Value &real_object) {
        real_object.ParseFromString(serialized_object);
    }

    static void save(const Value& real_object, std::string& serialized_object) {
        real_object.SerializeToString(&serialized_object);
    }
};

template<>
struct serialization_traits<PropertyValue> {

    static void load(const std::string& serialized_object, PropertyValue &real_object) {
        real_object.ParseFromString(serialized_object);
    }

    static void save(const PropertyValue& real_object, std::string& serialized_object) {
        real_object.SerializeToString(&serialized_object);
    }
};

template<>
struct serialization_traits<Statement> {

    static void load(const std::string& serialized_object, Statement &real_object) {
        real_object.ParseFromString(serialized_object);
    }

    static void save(const Statement& real_object, std::string& serialized_object) {
        real_object.SerializeToString(&serialized_object);
    }
};

template<>
struct serialization_traits<std::vector<Statement>> {

    static void load(const std::string& serialized_object,std::vector<Statement> &real_object) {
        archive a;
        a.str(serialized_object);
        cppcms::details::archive_load_container<std::vector<Statement>>(real_object, a);
    }

    static void save(const std::vector<Statement>& real_object,std::string &serialized_object) {
        archive a;
        cppcms::details::archive_save_container<std::vector<Statement>>(real_object, a);
        serialized_object = a.str();
    }
};

template<>
struct serialization_traits<std::vector<std::string>> {

    static void load(const std::string& serialized_object, std::vector<std::string> &real_object) {
        archive a;
        a.str(serialized_object);
        cppcms::details::archive_load_container<std::vector<std::string>>(real_object, a);
    }

    static void save(const std::vector<std::string>& real_object, std::string &serialized_object) {
        archive a;
        cppcms::details::archive_save_container<std::vector<std::string>>(real_object, a);
        serialized_object = a.str();
    }
};
}  // namespace cppcms

#endif //PRIMARYSOURCES_STATEMENTCACHING_H
