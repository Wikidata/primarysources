// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SerializerTSV.h"


std::string decimalToString(decimal_t decimal) {
    std::ostringstream stream;
    stream.precision(100);
    stream << std::showpos << std::fixed << decimal;
    std::string string = stream.str();

    //Removes trailing zeros
    string.erase(string.find_last_not_of('0') + 1, std::string::npos);

    //Removes the '.' if needed
    if(string.back() == '.') {
        string.pop_back();
    }
    return string;
}

namespace Serializer {

    static void writeValueTSV(const Value& v, std::ostream* out) {
        switch (v.getType()) {
            case ITEM:
                *out << v.getString();
                break;
            case LOCATION:
                *out << "@" << v.getLocation().first
                     << "/" << v.getLocation().second;
                break;
            case QUANTITY:
                *out << decimalToString(v.getQuantity());
                break;
            case STRING:
                if (v.getLanguage() != "") {
                    *out << v.getLanguage() << ":";
                }
                out->put('"');
                for (char c : v.getString()) {
                    if (c == '\\' || c == '"') {
                        out->put('\\');
                    }
                    out->put(c);
                }
                out->put('"');
                break;
            case TIME:
                Time time = v.getTime();
                *out << std::setfill('0')
                        << "+" << std::setw(4) << time.year
                        << "-" << std::setw(2) << time.month
                        << "-" << std::setw(2) << time.day
                        << "T" << std::setw(2) << time.hour
                        << ":" << std::setw(2) << time.minute
                        << ":" << std::setw(2) << time.second
                        << "Z/" << v.getPrecision();
                break;
        }
    }


    void writeStatementTSV(const Statement& stmt, std::ostream* out) {
        *out << stmt.getQID() << "\t"
             << stmt.getProperty() << "\t";
        writeValueTSV(stmt.getValue(), out);

        for (const PropertyValue& pv : stmt.getQualifiers()) {
            *out << "\t" << pv.getProperty() << "\t";
            writeValueTSV(pv.getValue(), out);
        }

        for (const PropertyValue& pv : stmt.getSources()) {
            *out << "\t" << pv.getProperty() << "\t";
            writeValueTSV(pv.getValue(), out);
        }
        *out << std::endl;
    }

    void writeStatementEnvelopeJSON(
            const Statement& stmt, cppcms::json::value* out) {
        cppcms::json::value entity;

        // write statement as TSV to a string value
        std::ostringstream sout;

        sout << stmt.getQID() << "\t"
             << stmt.getProperty() << "\t";
        writeValueTSV(stmt.getValue(), &sout);

        for (const PropertyValue& pv : stmt.getQualifiers()) {
            sout << "\t" << pv.getProperty() << "\t";
            writeValueTSV(pv.getValue(), &sout);
        }

        for (const PropertyValue& pv : stmt.getSources()) {
            sout << "\t" << pv.getProperty() << "\t";
            writeValueTSV(pv.getValue(), &sout);
        }

        (*out)["statement"] = sout.str();
        (*out)["id"] = stmt.getID();
        (*out)["format"] = "v1";
        (*out)["dataset"] = stmt.getDataset();
        (*out)["upload"] = stmt.getUpload();

        switch (stmt.getApprovalState()) {
            case UNAPPROVED: (*out)["state"] = "unapproved"; break;
            case APPROVED: (*out)["state"] = "approved"; break;
            case WRONG: (*out)["state"] = "wrong"; break;
            case OTHERSOURCE: (*out)["state"] = "othersource"; break;
            case SKIPPED: (*out)["state"] = "skipped"; break;
            case DUPLICATE: (*out)["state"] = "duplicate"; break;
            case BLACKLISTED: (*out)["state"] = "blacklisted"; break;
        }
    }

}  // namespace Serializer
