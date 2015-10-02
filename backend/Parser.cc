// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Parser.h"

#include <string>
#include <algorithm>    // copy
#include <utility>

#include <boost/tokenizer.hpp>
#include <boost/regex.hpp>

Value Parser::parseValue(const std::string& value) {
    // regular expressions for the different value formats

    // QID of entities, properties, sources
    static boost::regex re_entity("\\s*([QPS]\\d+)\\s*");

    // time format (e.g. +1967-01-17T00:00:00Z/11)
    static boost::regex re_time("\\s*\\+(\\d+)-(\\d{2})-(\\d{2})T(\\d{2}):(\\d{2}):(\\d{2})Z/(\\d{1,2})\\s*");

    // numbers (+/-1234.4567)
    static boost::regex re_num("\\s*(\\+|-)\\d+(\\.\\d+)?\\s*");

    // locations (@LAT/LONG)
    static boost::regex re_loc("\\s*@([+-]?\\d{1,2}(?:\\.\\d+)?)/([+-]?\\d{1,3}(?:\\.\\d+))?\\s*");

    // text (en:"He's a jolly good fellow")
    static boost::regex re_text("\\s*(?:(\\w{2}):)?\"([^\"\\\\]*(\\\\.[^\"\\\\]*)*)\"\\s*");

    boost::smatch sm;
    if (boost::regex_match(value, sm, re_entity)) {
        return Value(sm[1]);
    } else if (boost::regex_match(value, re_num)) {
        return Value(Quantity(value));
    } else if (boost::regex_match(value, sm, re_loc)) {
        return Value(std::stod(sm.str(1)), std::stod(sm.str(2)));
    } else if (boost::regex_match(value, sm, re_text)) {
        return Value(sm.str(2), sm.str(1));
    } else if (boost::regex_match(value, sm, re_time)) {
        return Value(Time(std::stoi(sm.str(1)), std::stoi(sm.str(2)), std::stoi(sm.str(3)),
                          std::stoi(sm.str(4)), std::stoi(sm.str(5)), std::stoi(sm.str(6)),
                          std::stoi(sm.str(7))));
    } else {
        return Value(value);
    }
}

PropertyValue parsePropertyValue(const std::string& property, const std::string& value) {
    return PropertyValue(property, Parser::parseValue(value));
}

void Parser::parseTSV(const std::string& dataset, int64_t upload,
                      std::istream &in, std::function<void(Statement)> handler) {
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
                          qualifiers, sources, dataset, upload, UNAPPROVED));
    }
}
