// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef HAVE_STATEMENT_H_
#define HAVE_STATEMENT_H_

#include <ctime>
#include <string>
#include <vector>

#include <cppcms/serialization.h>
#include <boost/multiprecision/cpp_dec_float.hpp>

enum ApprovalState {
    // Statement is a new import and has not yet been approved for Wikidata.
    UNAPPROVED,

    // Statement has already been approved for Wikidata.
    APPROVED,

    // TODO: unused.
    OTHERSOURCE,

    // Statement is wrong (e.g. wrong source, wrong fact).
    WRONG,

    // TODO: unused.
    SKIPPED,

    // Statement already exists in the database.
    DUPLICATE,

    // Statement has been blacklisted (e.g. because it comes from an untrustworthy source).
    BLACKLISTED,

    // Special state only used for querying purposes.
    ANY
};


// define decimal datatype to represent wikidata quantities as multi-
// precision
typedef boost::multiprecision::cpp_dec_float_50 Quantity;

// define locations as pairs of doubles
typedef std::pair<double, double> Location;

// Time structure more lenient than Time
class Time {
public:

    Time() {}

    explicit Time(int16_t year)
            : year(year), month(0), day(0),
              hour(0), minute(0), second(0),
              precision(9) { }

    Time(int16_t year, int8_t month)
            : year(year), month(month), day(0),
              hour(0), minute(0), second(0),
              precision(10) { }

    Time(int16_t year, int8_t month, int8_t day)
            : year(year), month(month), day(day),
              hour(0), minute(0), second(0),
              precision(11) { }

    Time(int16_t year, int8_t month, int8_t day,
         int8_t hour, int8_t minute)
            : year(year), month(month), day(day),
              hour(hour), minute(minute), second(0),
              precision(13) { }

    Time(int16_t year, int8_t month, int8_t day,
         int8_t hour, int8_t minute, int8_t second)
            : year(year), month(month), day(day),
              hour(hour), minute(minute), second(second),
              precision(14) { }

    Time(int16_t year, int8_t month, int8_t day,
         int8_t hour, int8_t minute, int8_t second, int8_t precision)
            : year(year), month(month), day(day),
              hour(hour), minute(minute), second(second),
              precision(precision) { }

    /**
     * Return a wikidata string representation of this time using the format described in
     * http://tools.wmflabs.org/wikidata-todo/quick_statements.php
     */
    std::string toWikidataString() const;

    /**
     * Return a SQL string representation of this time using the standard format used by
     * SQLLite and MySQL. Note that this representation does not take into account the
     * precision.
     */
    std::string toSQLString() const;

    int16_t year;
    int8_t month;
    int8_t day;
    int8_t hour;
    int8_t minute;
    int8_t second;

    // Wikidata precision specification.
    int8_t precision;

    friend bool operator==(const Time& lhs, const Time& rhs);
};

bool operator==(const Time& lhs, const Time& rhs);

inline bool operator!=(const Time& lhs, const Time& rhs) {
    return !(lhs == rhs);
}

/**
 * Representation of an activity entry for a statement. Each activity
 * is carried out by a user, changes the statement state to a new state,
 * and happened at a specific timepoint.
 */
class LogEntry : public cppcms::serializable {
public:

    LogEntry(const std::string &user, ApprovalState state, const Time &time)
            : user(user), state(state), time(time) { }

    const std::string &getUser() const {
        return user;
    }

    const ApprovalState &getState() const {
        return state;
    }

    const Time &getTime() const {
        return time;
    }

    void serialize(cppcms::archive &a) override;

private:
    LogEntry() { }

    std::string user;
    ApprovalState state;
    Time time;

    friend class cppcms::archive;
    friend void cppcms::details::archive_load_container<LogEntry>(LogEntry&, cppcms::archive&);
    friend void cppcms::details::archive_load_container<std::vector<LogEntry>>(std::vector<LogEntry>&, cppcms::archive&);
};


enum ValueType {
    ENTITY, STRING, TIME, LOCATION, QUANTITY
};

/**
* Representation of a Wikidata value. Wikidata values can be of several
* different types (see ValueType). This class serves as a container for
* all of them; in case they are not used they are left default initialized.
*
* The problem could potentially be solved more cleanly using inheritance,
* at the cost of having to introduce pointers and still needing to
* runtime-test for the different value types, so the merged solution is
* simpler.
*/
class Value : public cppcms::serializable {
 public:
    /**
    * Initialise a value of type ITEM using the QID passed as argument.
    */
    explicit Value(std::string qid) : str(qid), type(ENTITY) { }

    /**
    * Initialise a value of type STRING using the text and language
    * passed as argument. Strings without language use empty string as
    * language.
    */
    explicit Value(std::string s, std::string lang)
            : str(s), lang(lang), type(STRING) { }

    /**
    * Initialise a value of type TIME using the time structure and precision
    * given as argument.
    */
    explicit Value(Time t)
            : time(t), type(TIME) { }

    /**
    * Initialise a value if type LOCATION using the latitude and longitude
    * given as argument.
    */
    explicit Value(double lat, double lng)
            : loc(std::make_pair(lat, lng)), type(LOCATION) { }

    /**
    * Initialise a value of type QUANTITY using the multiprecision decimal
    * number given as argument.
    */
    explicit Value(Quantity d) : quantity(d), type(QUANTITY) { }

    // default copy constructor and assignment operator
    Value(const Value& other) = default;
    Value& operator=(const Value& other) = default;

    /**
    * Return the string value contained in this object. Only applicable to
    * values of type ITEM or STRING.
    */
    const std::string &getString() const {
        return str;
    }

    /**
    * Return the time value contained in this object. Only applicable to
    * values of type TIME.
    */
    const Time &getTime() const {
        return time;
    }

    /**
    * Return the latitude/longitude pair contained in this object. Only
    * applicable to values of type LOCATION.
    */
    const Location &getLocation() const {
        return loc;
    }

    /**
    * Return the decimal value contained in this object. Only
    * applicable to values of type QUANTITY.
    */
    const Quantity &getQuantity() const {
        return quantity;
    }

    /**
    * Return the decimal value contained in this object as string. Only
    * applicable to values of type QUANTITY.
    */
    std::string getQuantityAsString() const;

    /**
    * Return the language of this value.
    */
    const std::string &getLanguage() const {
        return lang;
    }

    /**
    * Return the type of this value.
    */
    const ValueType &getType() const {
        return type;
    }


    void serialize(cppcms::archive &a) override;

 private:
    Value() {}

    std::string str, lang;
    Time        time;
    Location    loc;
    Quantity    quantity;

    ValueType   type;

    // needed for (de-)serialization
    friend class PropertyValue;
    friend class cppcms::archive;
    friend class ValueTest_Serialize_Test;

    friend bool operator==(const Value& lhs, const Value& rhs);
};

bool operator==(const Value& lhs, const Value& rhs);

inline bool operator!=(const Value& lhs, const Value& rhs) {
    return !(lhs == rhs);
}

/**
* Property-value pair, also called 'snak' in Wikidata terminology. Used to
* represent the primary property and value, as well as qualifiers and sources.
*
* All value types are represented internally using the string representation
* described at http://tools.wmflabs.org/wikidata-todo/quick_statements.php
*/
class PropertyValue : public cppcms::serializable {
 public:
    PropertyValue(std::string property, Value value)
            : property(property), value(value) {  }

    // default copy constructor and assignment operator
    PropertyValue(const PropertyValue& other) = default;
    PropertyValue& operator=(const PropertyValue& other) = default;


    const std::string &getProperty() const {
        return property;
    }

    const Value &getValue() const {
        return value;
    }


    void serialize(cppcms::archive &a) override;

 private:
    PropertyValue() {}

    std::string property;

    Value value;

    // needed for (de-)serialization
    friend class Statement;
    friend class cppcms::archive;
    friend class PropertyValueTest_Serialize_Test;
    friend void cppcms::details::archive_load_container<PropertyValue>(PropertyValue&, cppcms::archive&);
    friend void cppcms::details::archive_load_container<std::vector<PropertyValue>>(std::vector<PropertyValue>&, cppcms::archive&);
    friend bool operator==(const PropertyValue& lhs, const PropertyValue& rhs);
};


bool operator==(const PropertyValue& lhs, const PropertyValue& rhs);

inline bool operator!=(const PropertyValue& lhs, const PropertyValue& rhs) {
    return !(lhs == rhs);
}


/**
* Representation of a Freebase/Wikidata statement to be approved by users.
* Similar to RDF triples, but a simplified model without blank nodes. For each
* statement we also store an internal (database) ID that allows to uniquely
* identify the statement, and the current state of approval.
*/
class Statement : public cppcms::serializable {
 public:
    typedef std::vector<PropertyValue> extensions_t;


    // main constructor; will be called usually in a parser or with database
    // results so we use copy by value and rely on the compiler to optimize
    // using rvalues
    Statement(int64_t id, std::string qid, PropertyValue propertyValue,
              extensions_t qualifiers, extensions_t sources,
              ApprovalState approved)
            : id(id), qid(qid), propertyValue(propertyValue),
              qualifiers(qualifiers), sources(sources), approved(approved) { }

    Statement(int64_t id, std::string qid, PropertyValue propertyValue,
              extensions_t qualifiers, extensions_t sources,
              std::string dataset, int64_t upload, ApprovalState approved,
              std::vector<LogEntry> activities = {})
            : id(id), qid(qid), propertyValue(propertyValue),
              qualifiers(qualifiers), sources(sources),
              dataset(dataset), upload(upload), approved(approved),
              activities(activities) { }

    Statement(std::string qid, PropertyValue propertyValue)
            : id(-1), qid(qid), propertyValue(propertyValue),
              approved(UNAPPROVED) {}

    // default copy constructor and assignment operator
    Statement(const Statement& other) = default;
    Statement& operator=(const Statement& other) = default;


    /**
    * Return the (internal) database ID of the statement. Used for unique
    * identification purposes within the system.
    */
    int64_t getID() const { return id; }

    /**
    * Return the Wikidata QID (subject) of the statement. Used to identify the
    * subject concept of the statement
    */
    const std::string& getQID() const { return qid; }

    /**
    * Return the Wikidata property of the statement.
    */
    const std::string& getProperty() const {
        return propertyValue.getProperty();
    }


    const Value &getValue() const {
        return propertyValue.getValue();
    }


    const PropertyValue &getPropertyValue() const {
        return propertyValue;
    }

    const extensions_t &getQualifiers() const {
        return qualifiers;
    }

    const extensions_t &getSources() const {
        return sources;
    }

    const std::string &getDataset() const {
        return dataset;
    }

    int64_t getUpload() const {
        return upload;
    }

    /**
    * Return true if this statement has already been approved, false otherwise.
    */
    ApprovalState getApprovalState() const { return approved; }


    void setApprovalState(ApprovalState const &approved) {
        Statement::approved = approved;
    }


    const std::vector<LogEntry> &getActivities() const {
        return activities;
    }

    void serialize(cppcms::archive &a) override;

 private:
    Statement() {}

    int64_t id;

    std::string qid;

    PropertyValue propertyValue;

    extensions_t qualifiers;
    extensions_t sources;

    // a string identifying the dataset this statement belongs to
    std::string dataset;

    // an integer id identifying the upload this statement was part of
    int64_t upload;

    ApprovalState approved;

    // Activity log for this statement. Populated from the database only, never
    // changed at runtime.
    std::vector<LogEntry> activities;

    // needed for (de-)serialization
    friend class cppcms::archive;
    friend class StatementTest_Serialize_Test;
    friend void cppcms::details::archive_load_container<Statement>(Statement&, cppcms::archive&);
    friend void cppcms::details::archive_load_container<std::vector<Statement>>(std::vector<Statement>&, cppcms::archive&);
    friend bool operator==(const Statement& lhs, const Statement& rhs);
};

// equality does not take into account approval state and database ID
bool operator==(const Statement& lhs, const Statement& rhs);

inline bool operator!=(const Statement& lhs, const Statement& rhs) {
    return !(lhs == rhs);
}


class InvalidApprovalState : public std::exception {
public:
    explicit InvalidApprovalState(const std::string &message)
            : message(message) { }

    const char *what() const noexcept override {
        return message.c_str();
    }

private:
    std::string message;
};

// TODO: define as operator? but it will throw an exception...
ApprovalState stateFromString(const std::string& state);
std::string stateToString(ApprovalState state);

#endif  // HAVE_STATEMENT_H_
