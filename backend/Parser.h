// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_PARSER_H_
#define HAVE_PARSER_H_

#include <iostream>

#include "Statement.h"

namespace Parser {

    /**
    * Parse the tab-separated value format for Wikidata statements described
    * here: http://tools.wmflabs.org/wikidata-todo/quick_statements.php .
    *
    * Statements are read from the input stream given as first argument. The
    * parser then creates a new Statement and directly passes it to the
    * handler function given as argument.
    */
    void parseTSV(std::istream &in, std::function<void(Statement)> handler);

}  // namespace Parser

#endif  // HAVE_PARSER_H_
