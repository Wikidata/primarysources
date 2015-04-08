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
    UNAPPROVED, APPROVED, OTHERSOURCE, WRONG, SKIPPED, DUPLICATE, BLACKLISTED
};

enum ValueType {
    ITEM, STRING, TIME, LOCATION, QUANTITY
};


// define decimal datatype to represent wikidata quantities as multi-
// precision
typedef boost::multiprecision::cpp_dec_float_50 decimal_t;

// define locations as pairs of doubles
typedef std::pair<double, double> location_t;

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
    explicit Value(std::string qid) : str(qid), type(ITEM) { }

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
    explicit Value(std::tm t, int precision)
            : time(t), precision(precision), type(TIME) { }

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
    explicit Value(decimal_t d) : quantity(d), type(QUANTITY) { }

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
    const std::tm &getTime() const {
        return time;
    }

    /**
    * Return the time precision contained in this object. Only applicable to
    * values of type TIME.
    */
    int getPrecision() const {
        return precision;
    }

    /**
    * Return the latitude/longitude pair contained in this object. Only
    * applicable to values of type LOCATION.
    */
    const location_t &getLocation() const {
        return loc;
    }

    /**
    * Return the decimal value contained in this object. Only
    * applicable to values of type QUANTITY.
    */
    const decimal_t &getQuantity() const {
        return quantity;
    }

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
    std::tm     time;
    location_t  loc;
    decimal_t   quantity;
    int         precision;

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

    /**
    * Return true if this statement has already been approved, false otherwise.
    */
    ApprovalState getApprovalState() const { return approved; }


    void serialize(cppcms::archive &a) override;

 private:
    Statement() {}

    int64_t id;

    std::string qid;

    PropertyValue propertyValue;

    extensions_t qualifiers;
    extensions_t sources;


    ApprovalState approved;

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


#endif  // HAVE_STATEMENT_H_
