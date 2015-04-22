// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Statement.h"

ApprovalState stateFromString(const std::string &state) {
    if (state == "approved") {
        return APPROVED;
    } else if(state == "wrong") {
        return WRONG;
    } else if(state == "skipped") {
        return SKIPPED;
    } else if(state == "othersource") {
        return OTHERSOURCE;
    } else if(state == "unapproved") {
        return UNAPPROVED;
    } else if(state == "duplicate") {
        return DUPLICATE;
    } else if(state == "blacklisted") {
        return BLACKLISTED;
    } else {
        throw InvalidApprovalState("Bad Request: invalid or missing state parameter ("+state+")");
    }
}

std::string stateToString(ApprovalState state) {
    return "";
}

void Value::serialize(cppcms::archive &a) {
    a & str & lang & cppcms::as_pod(time) & loc & precision
      & cppcms::as_pod(quantity) & cppcms::as_pod(type);
}

void PropertyValue::serialize(cppcms::archive &a) {
    a & property & value;
}

void Statement::serialize(cppcms::archive &a) {
    a & id & qid & propertyValue & qualifiers & sources
      & cppcms::as_pod(approved);
}

bool operator==(const Value& lhs, const Value& rhs) {
    if (lhs.type != rhs.type) return false;
    switch (lhs.type) {
        case ITEM:
            return lhs.str == rhs.str;
        case STRING:
            return lhs.str == rhs.str && lhs.lang == rhs.lang;
        case TIME:
            return lhs.time.tm_sec == rhs.time.tm_sec
                   && lhs.time.tm_min == rhs.time.tm_min
                   && lhs.time.tm_hour == rhs.time.tm_hour
                   && lhs.time.tm_mday == rhs.time.tm_mday
                   && lhs.time.tm_mon == rhs.time.tm_mon
                   && lhs.time.tm_year == rhs.time.tm_year
                   && lhs.time.tm_isdst == rhs.time.tm_isdst
                   && lhs.precision == rhs.precision;
        case QUANTITY:
            return lhs.quantity == rhs.quantity;
        case LOCATION:
            return lhs.loc == rhs.loc;
    }
    return false;
}

bool operator==(const PropertyValue& lhs, const PropertyValue& rhs) {
    return lhs.property == rhs.property && lhs.value == rhs.value;
}

bool operator==(const Statement& lhs, const Statement& rhs) {
    if (lhs.qid != rhs.qid) return false;
    if (lhs.propertyValue != rhs.propertyValue) return false;
    if (lhs.qualifiers.size() != rhs.qualifiers.size()) return false;
    if (lhs.sources.size() != rhs.sources.size()) return false;
    for (auto x : lhs.qualifiers) {
        bool found = false;
        for (auto y : rhs.qualifiers) {
            if (x == y) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    for (auto x : lhs.sources) {
        bool found = false;
        for (auto y : rhs.sources) {
            if (x == y) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}
