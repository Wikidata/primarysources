// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_STATEMENT_H_
#define HAVE_STATEMENT_H_

#include <ctime>
#include <string>
#include <vector>

#include <model/statement.pb.h>

namespace wikidata {
namespace primarysources {
namespace model {

class InvalidApprovalState : public std::exception {
 public:
    explicit InvalidApprovalState(const std::string &message)
            : message(message) { }

    const char *what() const noexcept override {
        return message.c_str();
    }

 private:
    std::string message;
};


// Create a new entity value.
Value NewValue(const std::string& qid);

// Create a new literal value (with language=
Value NewValue(const std::string& content, const std::string& language);

// Create a new location value.
Value NewValue(double lat, double lng);

// Create a new decimal quantity value.
Value NewQuantity(const std::string& decimal);
Value NewQuantity(long double decimal);

// Create a new time value.
Value NewTime(Time&& time);
Value NewTime(int32_t year, int32_t month, int32_t day,
              int32_t hour, int32_t minute, int32_t second, int32_t precision);

PropertyValue NewPropertyValue(const std::string& property, Value&& v);

LogEntry NewLogEntry(
        const std::string& user, ApprovalState state, Time&& time);

Statement NewStatement(std::string qid, PropertyValue&& propertyValue);
Statement NewStatement(
        int64_t id, std::string qid, PropertyValue&& propertyValue,
        std::vector<PropertyValue> qualifiers, std::vector<PropertyValue> sources,
        std::string dataset, int64_t upload, ApprovalState approved,
        std::vector<LogEntry> activities = {});

bool operator==(const Time &lhs, const Time &rhs);
inline bool operator!=(const Time &lhs, const Time &rhs) {
    return !(lhs == rhs);
}

bool operator==(const Location &lhs, const Location& rhs);
inline bool operator!=(const Location &lhs, const Location &rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const Quantity &lhs, const Quantity& rhs) {
    return lhs.decimal() == rhs.decimal();
}
inline bool operator!=(const Quantity &lhs, const Quantity &rhs) {
    return !(lhs == rhs);
}

inline bool operator==(const Entity &lhs, const Entity& rhs) {
    return lhs.qid() == rhs.qid();
}
inline bool operator!=(const Entity &lhs, const Entity &rhs) {
    return !(lhs == rhs);
}

bool operator==(const Literal &lhs, const Literal& rhs);
inline bool operator!=(const Literal &lhs, const Literal &rhs) {
    return !(lhs == rhs);
}

bool operator==(const Value &lhs, const Value &rhs);
inline bool operator!=(const Value &lhs, const Value &rhs) {
    return !(lhs == rhs);
}

bool operator==(const PropertyValue &lhs, const PropertyValue &rhs);
inline bool operator!=(const PropertyValue &lhs, const PropertyValue &rhs) {
    return !(lhs == rhs);
}


// equality does not take into account approval state and database ID
bool operator==(const Statement &lhs, const Statement &rhs);

inline bool operator!=(const Statement &lhs, const Statement &rhs) {
    return !(lhs == rhs);
}


// TODO: define as operator? but it will throw an exception...
ApprovalState stateFromString(const std::string &state);

std::string stateToString(ApprovalState state);

/**
 * Return a wikidata string representation of this time using the format described in
 * http://tools.wmflabs.org/wikidata-todo/quick_statements.php
 */
std::string toWikidataString(const Time &t);

/**
 * Return a SQL string representation of this time using the standard format used by
 * SQLLite and MySQL. Note that this representation does not take into account the
 * precision.
 */
std::string toSQLString(const Time &t);

}  // namespace model
}  // namespace primarysources
}  // namespace wikidata

#endif  // HAVE_STATEMENT_H_
