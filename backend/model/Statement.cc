// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>


#include <iomanip>
#include <sstream>

#include "Statement.h"

namespace wikidata {
namespace primarysources {
namespace model {

Value NewValue(const std::string& qid) {
    Value v;
    v.mutable_entity()->set_qid(qid);
    return std::move(v);
}

Value NewValue(const std::string& content, const std::string& language) {
    Value v;
    v.mutable_literal()->set_content(content);
    v.mutable_literal()->set_language(language);
    return std::move(v);
}

Value NewValue(double lat, double lng) {
    Value v;
    v.mutable_location()->set_latitude(lat);
    v.mutable_location()->set_longitude(lng);
    return std::move(v);
}

Value NewQuantity(const std::string& decimal) {
    std::string string = decimal;

    // Add sign if there is none.
    if (string[0] != '+' && string[0] != '-') {
        string.insert(string.begin(), '+');
    }

    Value v;
    v.mutable_quantity()->set_decimal(string);
    return std::move(v);
}

Value NewQuantity(long double decimal) {
    std::ostringstream stream;
    stream.precision(100);
    stream << std::showpos << std::fixed << decimal;

    std::string string = stream.str();
    // Removes trailing zeros
    string.erase(string.find_last_not_of('0') + 1, std::string::npos);

    // Removes the '.' if needed
    if(string.back() == '.') {
        string.pop_back();
    }

    return NewQuantity(string);
}


Value NewTime(Time&& time) {
    Value v;
    *v.mutable_time() = std::move(time);
    return std::move(v);
}

Value NewTime(int32_t year, int32_t month, int32_t day,
              int32_t hour, int32_t minute, int32_t second, int32_t precision) {
    Value v;
    v.mutable_time()->set_year(year);
    v.mutable_time()->set_month(month);
    v.mutable_time()->set_day(day);
    v.mutable_time()->set_hour(hour);
    v.mutable_time()->set_minute(minute);
    v.mutable_time()->set_second(second);
    v.mutable_time()->set_precision(precision);
    return std::move(v);
}

PropertyValue NewPropertyValue(
        const std::string& property, Value&& v) {
    PropertyValue pv;
    pv.set_property(property);
    pv.mutable_value()->Swap(&v);
    return std::move(pv);
}

LogEntry NewLogEntry(
        const std::string& user, ApprovalState state, Time&& time) {
    LogEntry l;
    l.set_user(user);
    l.set_state(state);
    l.mutable_time()->Swap(&time);
    return std::move(l);
}

Statement NewStatement(std::string qid, PropertyValue&& propertyValue) {
    Statement st;
    st.set_id(-1);
    st.set_qid(qid);
    st.set_approval_state(ApprovalState::UNAPPROVED);

    st.mutable_property_value()->Swap(&propertyValue);

    return st;
}

Statement NewStatement(
        int64_t id, std::string qid, PropertyValue&& propertyValue,
        std::vector<PropertyValue> qualifiers, std::vector<PropertyValue> sources,
        std::string dataset, int64_t upload, ApprovalState approved,
        std::vector<LogEntry> activities) {
    Statement st;
    st.set_id(id);
    st.set_qid(qid);
    st.set_dataset(dataset);
    st.set_upload(upload);
    st.set_approval_state(approved);

    st.mutable_property_value()->Swap(&propertyValue);

    for (const auto& pv : qualifiers) {
        *st.add_qualifiers() = pv;
    }
    for (const auto& pv : sources) {
        *st.add_sources() = pv;
    }
    for (const auto& l : activities) {
        *st.add_activities() = l;
    }

    return st;
}


bool operator==(const Time &lhs, const Time &rhs) {
    return lhs.year() == rhs.year() &&
           lhs.month() == rhs.month() &&
           lhs.day() == rhs.day() &&
           lhs.hour() == rhs.hour() &&
           lhs.minute() == rhs.minute() &&
           lhs.second() == rhs.second() &&
           lhs.precision() == rhs.precision();
}

bool operator==(const Location &lhs, const Location& rhs) {
    return lhs.latitude() == rhs.latitude() &&
           lhs.longitude() == rhs.longitude();
}

bool operator==(const Literal &lhs, const Literal& rhs) {
    return lhs.content() == rhs.content() &&
           lhs.language() == rhs.language();
}

bool operator==(const Value& lhs, const Value& rhs) {
    if (lhs.has_entity() && rhs.has_entity()) {
        return lhs.entity() == rhs.entity();
    }
    if (lhs.has_literal() && rhs.has_literal()) {
        return lhs.literal() == rhs.literal();
    }
    if (lhs.has_location() && rhs.has_location()) {
        return lhs.location() == rhs.location();
    }
    if (lhs.has_quantity() && rhs.has_quantity()) {
        return lhs.quantity() == rhs.quantity();
    }
    if (lhs.has_time() && rhs.has_time()) {
        return lhs.time() == rhs.time();
    }

    return false;
};

bool operator==(const PropertyValue &lhs, const PropertyValue &rhs) {
    return lhs.property() == rhs.property() &&
           lhs.value() == rhs.value();
}

bool operator==(const Statement &lhs, const Statement &rhs) {
    if (lhs.qid() != rhs.qid()) return false;
    if (lhs.property_value() != rhs.property_value()) return false;
    if (lhs.qualifiers().size() != rhs.qualifiers().size()) return false;
    if (lhs.sources().size() != rhs.sources().size()) return false;
    for (auto x : lhs.qualifiers()) {
        bool found = false;
        for (auto y : rhs.qualifiers()) {
            if (x == y) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }
    for (auto x : lhs.sources()) {
        bool found = false;
        for (auto y : rhs.sources()) {
            if (x == y) {
                found = true;
                break;
            }
        }
        if (!found) return false;
    }

    return true;
}


std::string toWikidataString(const Time& t) {
    std::ostringstream out;
    out << std::setfill('0')
        << "+" << std::setw(4) << (int) t.year()
        << "-" << std::setw(2) << (int) t.month()
        << "-" << std::setw(2) << (int) t.day()
        << "T" << std::setw(2) << (int) t.hour()
        << ":" << std::setw(2) << (int) t.minute()
        << ":" << std::setw(2) << (int) t.second()
        << "Z/" << (int) t.precision();

    return out.str();
}

std::string toSQLString(const Time& t) {
    std::ostringstream stream;
    stream << (int) t.year()
           << '-' << (int) t.month()
           << '-' << (int) t.day()
           << ' ' << (int) t.hour()
           << ':' << (int) t.minute()
           << ':' << (int) t.second();
    return stream.str();
}

ApprovalState stateFromString(const std::string &state) {
    if (state == "approved") {
        return APPROVED;
    } else if (state == "wrong") {
        return WRONG;
    } else if (state == "skipped") {
        return SKIPPED;
    } else if (state == "othersource") {
        return OTHERSOURCE;
    } else if (state == "unapproved") {
        return UNAPPROVED;
    } else if (state == "duplicate") {
        return DUPLICATE;
    } else if (state == "blacklisted") {
        return BLACKLISTED;
    } else if (state == "any") {
        return ANY;
    } else {
        throw InvalidApprovalState("Bad Request: invalid or missing state parameter (" + state + ")");
    }
}

std::string stateToString(ApprovalState state) {
    switch (state) {
        case APPROVED:
            return "approved";
        case UNAPPROVED:
            return "unapproved";
        case WRONG:
            return "wrong";
        case SKIPPED:
            return "skipped";
        case OTHERSOURCE:
            return "othersource";
        case DUPLICATE:
            return "duplicate";
        case BLACKLISTED:
            return "blacklisted";
        case ANY:
            return "any";
        default:
            return "";
    }
}

}  // namespace model
}  // namespace primarysources
}  // namespace wikidata
