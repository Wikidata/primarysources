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
        case ITEM: {
            const std::string &qid = pv.getValue().getString();
            snak["datavalue"]["type"] = "wikibase-entityid";
            switch (qid[0]) {
                case 'Q':
                    snak["datavalue"]["value"]["entity-type"] = "item";
                    break;
                case 'P':
                    snak["datavalue"]["value"]["entity-type"] = "property";
                    break;
                case 'S':
                    snak["datavalue"]["value"]["entity-type"] = "source";
                    break;
                default:
                    snak["datavalue"]["value"]["entity-type"] = "unknown";
                    break;
            }
            snak["datavalue"]["value"]["numeric-id"] =
                    atol(v.getString().c_str() + 1);
        }
            break;
        case STRING: {;
            const std::string &txt = v.getString();
            snak["datavalue"]["type"] = "string";
            snak["datavalue"]["value"] = txt;
            if (pv.getValue().getLanguage() != "") {
                // TODO(schaffert): is this correct? documentation doesn't
                // mention the case
                snak["datavalue"]["language"] = v.getLanguage();
            }
        }
            break;
        case LOCATION:
            snak["datavalue"]["type"] = "globecoordinate";
            snak["datavalue"]["value"]["latitude"] = v.getLocation().first;
            snak["datavalue"]["value"]["longitude"] = v.getLocation().second;
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
            snak["datavalue"]["value"]["precision"] = v.getPrecision();
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
