// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SerializerJSON.h"

#include <sstream>
#include <iomanip>

namespace wikidata {
namespace primarysources {
namespace serializer {

/**
* Create a Wikidata JSON 'snak', representing a property/value pair with
* metadata. Takes as argument an internal PropertyValue structure and
* returns a CppCMS JSON value for further use.
*/
static Json::Value createWikidataSnak(const model::PropertyValue& pv) {
    const model::Value& v = pv.value();

    Json::Value snak;
    snak["snaktype"] = "value";
    snak["property"] = pv.property();
    if (v.has_entity()) {
        const std::string &qid = v.entity().qid();
        snak["datavalue"]["type"] = "wikibase-entityid";
        switch (qid[0]) {
            case 'Q':
                snak["datavalue"]["value"]["entity-type"] = "item";
                break;
            case 'P':
                snak["datavalue"]["value"]["entity-type"] = "property";
                break;
            default:
                snak["datavalue"]["value"]["entity-type"] = "unknown";
                break;
        }
        snak["datavalue"]["value"]["numeric-id"] = (Json::Int64)atol(qid.c_str() + 1);
    } else if (v.has_literal()) {
        if (v.literal().language() == "") {
            snak["datavalue"]["type"] = "string";
            snak["datavalue"]["value"] = v.literal().content();
        } else {
            snak["datavalue"]["type"] = "monolingualtext";
            snak["datavalue"]["value"]["language"] = v.literal().language();
            snak["datavalue"]["value"]["text"] = v.literal().content();
        }
    } else if (v.has_location()) {
        snak["datavalue"]["type"] = "globecoordinate";
        snak["datavalue"]["value"]["latitude"] = v.location().latitude();
        snak["datavalue"]["value"]["longitude"] = v.location().longitude();
        snak["datavalue"]["value"]["globe"] = "http://www.wikidata.org/entity/Q2";
    } else if (v.has_time()) {
        std::ostringstream fmt;
        model::Time time = v.time();
        fmt << std::setfill('0')
            << "+" << std::setw(4) << time.year()
            << "-" << std::setw(2) << time.month()
            << "-" << std::setw(2) << time.day()
            << "T" << std::setw(2) << time.hour()
            << ":" << std::setw(2) << time.minute()
            << ":" << std::setw(2) << time.second()
            << "Z";
        snak["datavalue"]["type"] = "time";
        snak["datavalue"]["value"]["time"] = fmt.str();
        snak["datavalue"]["value"]["timezone"] = 0;
        snak["datavalue"]["value"]["before"] = 0;
        snak["datavalue"]["value"]["after"] = 0;
        snak["datavalue"]["value"]["precision"] = time.precision();
        snak["datavalue"]["value"]["calendarmodel"] = "http://www.wikidata.org/entity/Q1985727";
    } else if (v.has_quantity()) {
        const std::string& amount = v.quantity().decimal();
        snak["datavalue"]["type"] = "quantity";
        snak["datavalue"]["value"]["amount"] = amount;
        snak["datavalue"]["value"]["unit"] = "1";
        snak["datavalue"]["value"]["upperBound"] = amount;
        snak["datavalue"]["value"]["lowerBound"] = amount;
    }
    return snak;
}


void writeStatementWikidataJSON(
        const model::Statement& stmt, Json::Value* entities) {
    const std::string& prop = stmt.property_value().property();
    const std::string& qid  = stmt.qid();

    Json::Value entity;

    entity["id"] = stmt.id();
    entity["type"] = "claim";
    entity["rank"] = "normal";
    entity["mainsnak"] = createWikidataSnak(stmt.property_value());

    std::map<std::string, int> qcount;
    for (const model::PropertyValue& pv : stmt.qualifiers()) {
        int c = 0;

        auto it = qcount.find(pv.property());
        if (it != qcount.end()) {
            c = it->second;
        }

        entity["qualifiers"][pv.property()][c] = createWikidataSnak(pv);

        qcount[pv.property()] = ++c;
    }

    std::map<std::string, int> scount;
    for (const model::PropertyValue& pv : stmt.sources()) {
        int c = 0;

        auto it = scount.find(pv.property());
        if (it != scount.end()) {
            c = it->second;
        }

        entity["references"][0]["snaks"][pv.property()][c] =
                createWikidataSnak(pv);

        scount[pv.property()] = ++c;
    }


    (*entities)[qid]["claims"][prop].append(entity);
}

}  // namespace serializer
}  // namespace primarysources
}  // namespace wikidata
