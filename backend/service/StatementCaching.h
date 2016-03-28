//
// Created by wastl on 25.03.16.
//

#ifndef PRIMARYSOURCES_STATEMENTCACHING_H
#define PRIMARYSOURCES_STATEMENTCACHING_H

#include <cppcms/cache_interface.h>
#include <model/Statement.h>
#include <model/Status.h>

namespace cppcms {

using ::wikidata::primarysources::model::PropertyValue;
using ::wikidata::primarysources::model::Statement;
using ::wikidata::primarysources::model::Value;

using ::wikidata::primarysources::model::Statements;
using ::wikidata::primarysources::model::Strings;

using ::wikidata::primarysources::model::Status;

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
        Statements stmts;
        stmts.ParseFromString(serialized_object);
        for (auto& s : stmts.statements()) {
            real_object.emplace_back(std::move(s));
        }
    }

    static void save(const std::vector<Statement>& real_object,std::string &serialized_object) {
        Statements stmts;
        for (const auto& s : real_object) {
            *stmts.add_statements() = s;
        }
        stmts.SerializeToString(&serialized_object);
    }
};

template<>
struct serialization_traits<std::vector<std::string>> {

    static void load(const std::string& serialized_object, std::vector<std::string> &real_object) {
        Strings stmts;
        stmts.ParseFromString(serialized_object);
        for (auto& s : stmts.strings()) {
            real_object.emplace_back(std::move(s));
        }
    }

    static void save(const std::vector<std::string>& real_object, std::string &serialized_object) {
        Strings stmts;
        for (const auto& s : real_object) {
            *stmts.add_strings() = s;
        }
        stmts.SerializeToString(&serialized_object);
    }
};

template<>
struct serialization_traits<Status> {

    static void load(const std::string& serialized_object, Status &real_object) {
        real_object.ParseFromString(serialized_object);
    }

    static void save(const Status& real_object, std::string& serialized_object) {
        real_object.SerializeToString(&serialized_object);
    }
};


}  // namespace cppcms

#endif //PRIMARYSOURCES_STATEMENTCACHING_H
