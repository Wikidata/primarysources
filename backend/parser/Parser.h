// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_PARSER_H_
#define HAVE_PARSER_H_

#include <iostream>

#include "model/Statement.h"

namespace wikidata {
namespace primarysources {
namespace parser {

    /**
    * Parse the tab-separated value format for Wikidata statements described
    * here: http://tools.wmflabs.org/wikidata-todo/quick_statements.php .
    *
    * Statements are read from the input stream given as first argument. The
    * parser then creates a new Statement and directly passes it to the
    * handler function given as argument.
    */
    void parseTSV(const std::string& dataset, int64_t upload,
                  std::istream &in, std::function<void(Statement)> handler);

    /**
    * A value formatted in TSV.
    */
    Value parseValue(const std::string& value);

}  // namespace parser
}  // namespace primarysources
}  // namespace wikidata
#endif  // HAVE_PARSER_H_
