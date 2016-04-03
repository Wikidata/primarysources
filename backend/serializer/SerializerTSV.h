// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_SERIALIZER_TSV_H_
#define HAVE_SERIALIZER_TSV_H_

#include <util/json/json.h>

#include "model/Statement.h"

namespace wikidata {
namespace primarysources {
namespace serializer {

    void writeStatementTSV(const model::Statement& stmt, std::ostream* out);

    void writeStatementEnvelopeJSON(
            const model::Statement& stmt, Json::Value* out);

    /**
    * Write a sequence of statements to the output stream using the TSV format
    * used by Wikidata.
    * (http://tools.wmflabs.org/wikidata-todo/quick_statements.php)
    */
    template<typename Iterator>
    void writeTSV(Iterator begin, Iterator end, std::ostream* out) {
        for (; begin != end; ++begin) {
            writeStatementTSV(*begin, out);
        }
    }

    /**
    * Write a sequence of statements to the output stream using the envelope
    * JSON format suggested by Denny. Example:
    * {
    *    "statement": "Q21 P23 Q12",
    *    "id" : 123,
    *    "format": "v1"
    * }
    *
    */
    template<typename Iterator>
    void writeEnvelopeJSON(Iterator begin, Iterator end, std::ostream* out) {
        // Force type to array so we get an empty list in case the iterator
        // is empty.
        Json::Value result(Json::arrayValue);

        for (int count = 0; begin != end; ++begin, ++count) {
            writeStatementEnvelopeJSON(*begin, &result[count]);
        }

        *out << result;
    }

}  // namespace serializer
}  // namespace primarysources
}  // namespace wikidata
#endif  // HAVE_SERIALIZER_TSV_H_
