// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Parser.h"

#include <string>
#include <algorithm>    // copy
#include <utility>

#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

PropertyValue parsePropertyValue(std::string property, std::string value) {
    // regular expressions for the different value formats

    // QID of entities, properties, sources
    static boost::regex re_entity("\\s*([QPS]\\d+)\\s*");

    // time format (e.g. +00000001967-01-17T00:00:00Z/11)
    static boost::regex re_time("\\s*\\+(\\d{11})-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})Z/(\\d{1,2})\\s*");

    // numbers (+/-1234.4567)
    static boost::regex re_num("\\s*(\\+|-)\\d+(\\.\\d+)?\\s*");

    // locations (@LAT/LONG)
    static boost::regex re_loc("\\s*@(\\d{1,2}(?:\\.\\d+)?)/(\\d{1,3}(?:\\.\\d+))?\\s*");

    // text (en:"He's a jolly good fellow")
    static boost::regex re_text("\\s*(?:(\\w{2}):)?\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\"\\s*");

    boost::smatch sm;
    if (boost::regex_match(value, sm, re_entity)) {
        return PropertyValue(property, Value(sm[1]));
    } else if (boost::regex_match(value, re_num)) {
        return PropertyValue(property, Value(decimal_t(value)));
    } else if (boost::regex_match(value, sm, re_loc)) {
        return PropertyValue(
                property,
                Value(std::stod(sm.str(1)), std::stod(sm.str(2))));
    } else if (boost::regex_match(value, sm, re_text)) {
        return PropertyValue(property, Value(sm.str(2), sm.str(1)));
    } else if (boost::regex_match(value, sm, re_time)) {
        std::tm time = {
                // sec, min, hour
                std::stoi(sm[6]), std::stoi(sm[5]), std::stoi(sm[4]),
                // day, mon, year
                std::stoi(sm[3]), std::stoi(sm[2]), std::stoi(sm[1]),
                0, 0, 0
            };
        return PropertyValue(property, Value(time, std::stoi(sm.str(7))));
    } else {
        return PropertyValue(property, Value(value));
    }
}

void Parser::parseTSV(std::istream &in, std::function<void(Statement)> handler) {
    typedef boost::tokenizer< boost::char_separator<char> > Tokenizer;
    boost::char_separator<char> sep("\t");

    std::vector< std::string > vec;
    std::string line;

    while (std::getline(in, line)) {
        Tokenizer tok(line, sep);
        vec.assign(tok.begin(), tok.end());

        Statement::extensions_t qualifiers, sources;

        // 0, 1, 2 are subject, predicate and object; everything that follows
        // is a qualifier or source, depending on first letter of predicate.
        for (int i=3; i+1 < vec.size(); i += 2) {
            if (vec[i][0] == 'S') {
                sources.push_back(parsePropertyValue(vec[i], vec[i+1]));
            } else {
                qualifiers.push_back(parsePropertyValue(vec[i], vec[i+1]));
            }
        }

        handler(Statement(-1, vec[0], parsePropertyValue(vec[1], vec[2]),
                          qualifiers, sources, UNAPPROVED));
    }
}
