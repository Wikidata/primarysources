// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SerializerJSON.h"

namespace Serializer {

/**
* Create a Wikidata JSON 'snak', representing a property/value pair with
* metadata. Takes as argument an internal PropertyValue structure and
* returns a CppCMS JSON value for further use.
*/
static cppcms::json::value createWikidataSnak(const PropertyValue& pv) {
    const Value& v = pv.getValue();

    cppcms::json::value snak;
    snak["snaktype"] = "value";
    snak["property"] = pv.getProperty();
    switch (v.getType()) {
        case ENTITY: {
            const std::string &qid = v.getString();
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
            snak["datavalue"]["value"]["numeric-id"] =
                    atol(qid.c_str() + 1);
        }
            break;
        case STRING:
            if (v.getLanguage() == "") {
                snak["datavalue"]["type"] = "string";
                snak["datavalue"]["value"] = v.getString();
            } else {
                snak["datavalue"]["type"] = "monolingualtext";
                snak["datavalue"]["value"]["language"] = v.getLanguage();
                snak["datavalue"]["value"]["text"] = v.getString();
            }
            break;
        case LOCATION:
            snak["datavalue"]["type"] = "globecoordinate";
            snak["datavalue"]["value"]["latitude"] = v.getLocation().first;
            snak["datavalue"]["value"]["longitude"] = v.getLocation().second;
            snak["datavalue"]["value"]["globe"] = "http://www.wikidata.org/entity/Q2";
            break;
        case TIME: {
            std::ostringstream fmt;
            Time time = v.getTime();
            fmt << std::setfill('0')
                    << "+" << std::setw(4) << time.year
                    << "-" << std::setw(2) << time.month
                    << "-" << std::setw(2) << time.day
                    << "T" << std::setw(2) << time.hour
                    << ":" << std::setw(2) << time.minute
                    << ":" << std::setw(2) << time.second
                    << "Z";
            snak["datavalue"]["type"] = "time";
            snak["datavalue"]["value"]["time"] = fmt.str();
            snak["datavalue"]["value"]["timezone"] = 0;
            snak["datavalue"]["value"]["before"] = 0;
            snak["datavalue"]["value"]["after"] = 0;
            snak["datavalue"]["value"]["precision"] = v.getPrecision();
            snak["datavalue"]["value"]["calendarmodel"] = "http://www.wikidata.org/entity/Q1985727";
        }
            break;
        case QUANTITY: {
            const std::string& amount = v.getQuantityAsString();
            snak["datavalue"]["type"] = "quantity";
            snak["datavalue"]["value"]["amount"] = amount;
            snak["datavalue"]["value"]["unit"] = "1";
            snak["datavalue"]["value"]["upperBound"] = amount;
            snak["datavalue"]["value"]["lowerBound"] = amount;
        }
            break;
    }
    return snak;
}


void writeStatementWikidataJSON(
        const Statement& stmt, cppcms::json::value* entities) {
    const std::string& prop = stmt.getProperty();
    const std::string& qid  = stmt.getQID();

    cppcms::json::value entity;

    entity["id"] = stmt.getID();
    entity["type"] = "claim";
    entity["rank"] = "normal";
    entity["mainsnak"] = createWikidataSnak(stmt.getPropertyValue());

    std::map<std::string, int> qcount;
    for (const PropertyValue& pv : stmt.getQualifiers()) {
        int c = 0;

        auto it = qcount.find(pv.getProperty());
        if (it != qcount.end()) {
            c = it->second;
        }

        entity["qualifiers"][pv.getProperty()][c] = createWikidataSnak(pv);

        qcount[pv.getProperty()] = ++c;
    }

    std::map<std::string, int> scount;
    for (const PropertyValue& pv : stmt.getSources()) {
        int c = 0;

        auto it = scount.find(pv.getProperty());
        if (it != scount.end()) {
            c = it->second;
        }

        entity["references"][0]["snaks"][pv.getProperty()][c] =
                createWikidataSnak(pv);

        scount[pv.getProperty()] = ++c;
    }


    (*entities)[qid]["claims"][prop] = entity;
}

}  // namespace Serializer
