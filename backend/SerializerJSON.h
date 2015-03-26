// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_SERIALIZERJSON_H_
#define HAVE_SERIALIZERJSON_H_

#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>

#include <cppcms/json.h>

#include "Statement.h"

namespace Serializer {

    /**
    * Add a statement to the Wikidata JSON representation passed as output
    * parameter. Uses the WikiData JSON format documented at
    * https://www.mediawiki.org/wiki/Wikibase/Notes/JSON .
    * Each statement is represented as a "claim" in the Wikidata terminology.
    */
    void writeStatementWikidataJSON(
            const Statement& stmt, cppcms::json::value* result);


    /**
    * Write a collection of statements to an output stream as JSON.
    * Uses the WikiData JSON format documented at
    * https://www.mediawiki.org/wiki/Wikibase/Notes/JSON .
    * Each statement is represented as a "claim" in the Wikidata terminology.
    */
    template<typename Iterator>
    void writeWikidataJSON(Iterator begin, Iterator end, std::ostream* out) {
        cppcms::json::value entities;

        for (; begin != end; ++begin) {
            writeStatementWikidataJSON(*begin, &entities);
        }

        cppcms::json::value result;
        result["entities"] = entities;
        result.save(*out, cppcms::json::readable);
    }

}  // namespace Serializer
#endif  // HAVE_SERIALIZERJSON_H_
