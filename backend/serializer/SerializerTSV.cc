// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SerializerTSV.h"

namespace wikidata {
namespace primarysources {
namespace serializer {

    static void writeValueTSV(const model::Value& v, std::ostream* out) {
        if (v.has_entity()) {
            *out << v.entity().qid();
        } else if (v.has_location()) {
            *out << "@" << v.location().latitude()
                 << "/" << v.location().longitude();
        } else if (v.has_quantity()) {
            *out << v.quantity().decimal();
        } else if (v.has_literal()) {
            if (v.literal().language() != "") {
                *out << v.literal().language() << ":";
            }
            out->put('"');
            for (char c : v.literal().content()) {
                if (c == '\\' || c == '"') {
                    out->put('\\');
                }
                out->put(c);
            }
            out->put('"');
        } else if (v.has_time()) {
            *out << model::toWikidataString(v.time());
        }
    }


    void writeStatementTSV(const model::Statement& stmt, std::ostream* out) {
        *out << stmt.qid() << "\t"
             << stmt.property_value().property() << "\t";
        writeValueTSV(stmt.property_value().value(), out);

        for (const auto& pv : stmt.qualifiers()) {
            *out << "\t" << pv.property() << "\t";
            writeValueTSV(pv.value(), out);
        }

        for (const auto& pv : stmt.sources()) {
            *out << "\t" << pv.property() << "\t";
            writeValueTSV(pv.value(), out);
        }
        *out << std::endl;
    }

    void writeStatementEnvelopeJSON(
            const model::Statement& stmt, cppcms::json::value* out) {
        cppcms::json::value entity;

        // write statement as TSV to a string value
        std::ostringstream sout;

        sout << stmt.qid() << "\t"
             << stmt.property_value().property() << "\t";
        writeValueTSV(stmt.property_value().value(), &sout);

        for (const auto& pv : stmt.qualifiers()) {
            sout << "\t" << pv.property() << "\t";
            writeValueTSV(pv.value(), &sout);
        }

        for (const auto& pv : stmt.sources()) {
            sout << "\t" << pv.property() << "\t";
            writeValueTSV(pv.value(), &sout);
        }

        (*out)["statement"] = sout.str();
        (*out)["id"] = stmt.id();
        (*out)["format"] = std::string("v1");
        (*out)["dataset"] = stmt.dataset();
        (*out)["upload"] = stmt.upload();
        (*out)["state"] = model::stateToString(stmt.approval_state());

        for (int i=0; i< stmt.activities().size(); i++) {
            const auto& entry = stmt.activities(i);
            (*out)["activities"][i]["user"] = entry.user();
            (*out)["activities"][i]["state"] = model::stateToString(entry.state());
            (*out)["activities"][i]["timestamp"] = model::toWikidataString(entry.time());
        }
    }

}  // namespace serializer
}  // namespace primarysources
}  // namespace wikidata
