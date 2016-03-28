// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Parser.h"

#include <re2/re2.h>

using wikidata::primarysources::model::ApprovalState ;
using wikidata::primarysources::model::PropertyValue;
using wikidata::primarysources::model::Statement;
using wikidata::primarysources::model::Value;

using wikidata::primarysources::model::NewQuantity;
using wikidata::primarysources::model::NewValue;
using wikidata::primarysources::model::NewTime;
using wikidata::primarysources::model::NewStatement;

namespace wikidata {
namespace primarysources {
namespace parser {
namespace {
// regular expressions for the different value formats

// QID of entities, properties, sources
RE2 re_entity("\\s*([QPS]\\d+)\\s*");

// time format (e.g. +1967-01-17T00:00:00Z/11)
RE2 re_time("\\s*\\+(\\d+)-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})Z/(\\d{1,2})\\s*");

// numbers (+/-1234.4567)
RE2 re_num("\\s*(\\+|-)\\d+(\\.\\d+)?\\s*");

// locations (@LAT/LONG)
RE2 re_loc("\\s*@([+-]?\\d{1,2}(?:\\.\\d+)?)/([+-]?\\d{1,3}(?:\\.\\d+))?\\s*");

// text (en:"He's a jolly good fellow")
RE2 re_text("\\s*(?:(\\w{2}):)?\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\"\\s*");

}  // namespace

std::vector<std::string> split(const char *str, char c = ' ') {
    std::vector<std::string> result;

    do {
        const char *begin = str;

        while (*str != c && *str)
            str++;

        result.push_back(std::string(begin, str));
    } while (0 != *str++);

    return result;
}

Value parseValue(const std::string& value) {

    std::string s, t;
    double d, e;
    int year, month, day, hour, minute, second, precision;
    if (RE2::FullMatch(value, re_entity, &s)) {
        return NewValue(s);
    } else if (RE2::FullMatch(value, re_num)) {
        return NewQuantity(value);
    } else if (RE2::FullMatch(value, re_loc, &d, &e)) {
        return NewValue(d, e);
    } else if (RE2::FullMatch(value, re_text, &s, &t)) {
        return NewValue(t, s);
    } else if (RE2::FullMatch(value, re_time,
                              &year, &month, &day,
                              &hour, &minute, &second, &precision)) {
        return NewTime(year, month, day, hour, minute, second, precision);
    } else {
        return NewValue(value);
    }
}

PropertyValue parsePropertyValue(const std::string& property, const std::string& value) {
    return NewPropertyValue(property, parseValue(value));
}

void parseTSV(const std::string& dataset, int64_t upload,
                      std::istream &in, std::function<void(Statement)> handler) {
    std::vector< std::string > vec;
    std::string line;

    while (std::getline(in, line)) {
        vec = split(line.c_str(), '\t');

        std::vector<PropertyValue> qualifiers, sources;

        // 0, 1, 2 are subject, predicate and object; everything that follows
        // is a qualifier or source, depending on first letter of predicate.
        for (int i=3; i+1 < vec.size(); i += 2) {
            if (vec[i][0] == 'S') {
                sources.push_back(parsePropertyValue(vec[i], vec[i+1]));
            } else {
                qualifiers.push_back(parsePropertyValue(vec[i], vec[i+1]));
            }
        }

        handler(NewStatement(-1, vec[0], parsePropertyValue(vec[1], vec[2]),
                             qualifiers, sources, dataset, upload, ApprovalState::UNAPPROVED));
    }
}
}  // namespace parser
}  // namespace primarysources
}  // namespace wikidata
