// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <sstream>
#include "Persistence.h"

using wikidata::primarysources::model::Location;
using wikidata::primarysources::model::Quantity;
using wikidata::primarysources::model::Time;
using wikidata::primarysources::model::Literal;

using wikidata::primarysources::model::ApprovalState;
using wikidata::primarysources::model::LogEntry;
using wikidata::primarysources::model::PropertyValue;
using wikidata::primarysources::model::Statement;
using wikidata::primarysources::model::Value;

using wikidata::primarysources::model::NewQuantity;
using wikidata::primarysources::model::NewTime;
using wikidata::primarysources::model::NewValue;
using wikidata::primarysources::model::NewPropertyValue;

namespace wikidata {
namespace primarysources {
namespace {

std::string build_mysql_connection(
        const std::string &db_name,
        const std::string &db_host, const std::string &db_port,
        const std::string &db_user, const std::string &db_pass
) {
    std::ostringstream out;
    out << "mysql:database=" << db_name;
    if (db_host != "") {
        out << ";host=" << db_host;
    }
    if (db_port != "") {
        out << ";port=" << db_port;
    }
    if (db_user != "") {
        out << ";user=" << db_user;
    }
    if (db_pass != "") {
        out << ";password=" << db_pass;
    }
    return out.str();
}

std::string build_sqlite_connection(const std::string &db_name) {
    std::ostringstream out;
    out << "sqlite3:db=" << db_name;
    return out.str();
}

inline ApprovalState getApprovalState(int16_t state) {
    switch (state) {
        case 0:
            return ApprovalState::UNAPPROVED;
        case 1:
            return ApprovalState::APPROVED;
        case 2:
            return ApprovalState::OTHERSOURCE;
        case 3:
            return ApprovalState::WRONG;
        case 4:
            return ApprovalState::SKIPPED;
        case 5:
            return ApprovalState::DUPLICATE;
        case 6:
            return ApprovalState::BLACKLISTED;
        default:
            return ApprovalState::UNAPPROVED;
    }
}

inline int16_t getSQLState(ApprovalState state) {
    switch (state) {
        case ApprovalState::UNAPPROVED:
            return 0;
        case ApprovalState::APPROVED:
            return 1;
        case ApprovalState::OTHERSOURCE:
            return 2;
        case ApprovalState::WRONG:
            return 3;
        case ApprovalState::SKIPPED:
            return 4;
        case ApprovalState::DUPLICATE:
            return 5;
        case ApprovalState::BLACKLISTED:
            return 6;
        case ApprovalState::ANY:
            return -1; // Special query only case.
    }
}

Time timeFromSql(const std::string &str) {
    Time time;

    short year;
    char month, day, hour, minute, second;
    if (sscanf(str.c_str(), "%hd-%hhd-%hhd %hhd:%hhd:%hhd",
               &year, &month, &day, &hour, &minute, &second) != 6) {
        throw PersistenceException("Invalid time: " + str);
    }
    time.set_year(year);
    time.set_month(month);
    time.set_day(day);
    time.set_hour(hour);
    time.set_minute(minute);
    time.set_second(second);
    time.set_precision(14);

    return time;
}

}  // namespace

std::string build_connection(const cppcms::json::value &config) {
    std::string driver = config["driver"].str();

    if (driver == "mysql") {
        return build_mysql_connection(config["name"].str(),
                                      config["host"].str(), config["port"].str(),
                                      config["user"].str(), config["password"].str());
    } else if (driver == "sqlite3") {
        return build_sqlite_connection(config["name"].str());
    } else {
        throw PersistenceException("Unknown driver:" + driver);
    }
}

long double Persistence::addSnak(const PropertyValue &pv) {
    if (pv.value().has_entity()) {
        return (sql << "INSERT INTO snak(property,svalue,vtype) "
                       "VALUES (?,?,'item')"
                    << pv.property() << pv.value().entity().qid()
                    << cppdb::exec).last_insert_id();
    } else if (pv.value().has_literal()) {
        return (sql << "INSERT INTO snak(property,svalue,lang,vtype) "
                       "VALUES (?,?,?,'string')"
                    << pv.property() << pv.value().literal().content()
                    << pv.value().literal().language() << cppdb::exec)
                    .last_insert_id();
    } else if (pv.value().has_quantity()) {
        return (sql << "INSERT INTO snak(property,dvalue,vtype) "
                       "VALUES (?,?,'quantity')"
                    << pv.property()
                    << std::stold(pv.value().quantity().decimal())
                    << cppdb::exec).last_insert_id();
    } else if (pv.value().has_time()) {
        if (pv.value().time().year() < 1000) {
            throw PersistenceException("Time values before year 1000 are not supported");
        }
        return (sql << "INSERT INTO snak(property,tvalue,`precision`,vtype) "
                       "VALUES (?,?,?,'time')"
                    << pv.property() << model::toSQLString(pv.value().time())
                    << pv.value().time().precision() << cppdb::exec)
                    .last_insert_id();
    } else if (pv.value().has_location()) {
        return (sql << "INSERT INTO snak(property,lat,lng,vtype) "
                       "VALUES (?,?,?,'location')"
                    << pv.property() << pv.value().location().latitude()
                    << pv.value().location().longitude() << cppdb::exec)
                .last_insert_id();
    }

    return -1;
}

int64_t Persistence::getSnakID(const PropertyValue &pv) {
    cppdb::result r;
    if (pv.value().has_entity()) {
        r = (sql << "SELECT id FROM snak "
                    "WHERE property=? AND svalue=? AND vtype='item'"
                 << pv.property() << pv.value().entity().qid()
                 << cppdb::row);
    } else if (pv.value().has_literal()) {
        r = (sql << "SELECT id FROM snak "
                    "WHERE property=? AND svalue=? AND lang=? "
                    "AND vtype='string'"
                 << pv.property() << pv.value().literal().content()
                 << pv.value().literal().language() << cppdb::row);
    } else if (pv.value().has_quantity()) {
        r= (sql << "SELECT id FROM snak "
                   "WHERE property=? AND dvalue=? AND vtype='quantity'"
                << pv.property()
                << std::stold(pv.value().quantity().decimal())
                << cppdb::row);
    } else if (pv.value().has_time()) {
        r = (sql << "SELECT id FROM snak "
                    "WHERE property=? AND tvalue=? AND `precision`=? "
                    "AND vtype='time'"
                 << pv.property() << model::toSQLString(pv.value().time())
                 << pv.value().time().precision() << cppdb::row);
    } else if (pv.value().has_location()) {
        r = (sql << "SELECT id FROM snak "
                    "WHERE property=? AND lat=? AND lng=? "
                    "AND vtype='location' "
                 << pv.property() << pv.value().location().latitude()
                 << pv.value().location().longitude() << cppdb::row);
    }

    if (!r.empty()) {
        return r.get<int64_t>("id");
    } else {
        return -1;
    }
}

PropertyValue Persistence::getSnak(int64_t snakid) {
    cppdb::result res =(
            sql << "SELECT property, svalue, dvalue, tvalue, "
                   "       lat, lng, `precision`, lang, vtype "
                   "FROM snak WHERE id = ?"
                << snakid << cppdb::row);

    if (!res.empty()) {
        std::string vtype = res.get<std::string>("vtype");
        std::string prop  = res.get<std::string>("property");
        if (vtype == "item") {
            std::string svalue = res.get<std::string>("svalue");
            return NewPropertyValue(prop, NewValue(svalue));
        } else if (vtype == "string") {
            std::string svalue = res.get<std::string>("svalue");
            std::string lang = res.get<std::string>("lang");
            return NewPropertyValue(prop, NewValue(svalue, lang));
        } else if (vtype == "time") {
            Time tvalue = timeFromSql(res.get<std::string>("tvalue"));
            tvalue.set_precision(res.get<int>("precision"));
            return NewPropertyValue(prop, NewTime(std::move(tvalue)));
        } else if (vtype == "location") {
            double lat = res.get<double>("lat");
            double lng = res.get<double>("lng");
            return NewPropertyValue(prop, NewValue(lat, lng));
        } else if (vtype == "quantity") {
            std::string dvalue = res.get<std::string>("dvalue");
            return NewPropertyValue(prop, NewQuantity(dvalue));
        } else {
            // no result found
            throw PersistenceException("snak has unknown type");
        }

    } else {
        // no result found
        throw PersistenceException("snak not found");
    }
}

std::vector<LogEntry> Persistence::getLogEntries(int64_t stmtid) {
    std::vector<LogEntry> result;
    cppdb::result res =(
            sql << "SELECT user, state, changed "
                    "FROM userlog WHERE stmt = ? "
                    "ORDER BY id ASC"
                << stmtid);

    while (res.next()) {
        result.push_back(NewLogEntry(
                res.get<std::string>("user"),
                getApprovalState(res.get<int16_t>("state")),
                timeFromSql(res.get<std::string>("changed"))
        ));
    }

    return result;
}


Statement Persistence::buildStatement(int64_t id, const std::string& qid,
                                      int64_t snak, const std::string& dataset,
                                      int64_t upload, int16_t state) {
    std::vector<PropertyValue> qualifiers, sources;

    // qualifiers
    cppdb::result qres =
            (sql << "SELECT snak FROM qualifier WHERE stmt = ?" << id);
    while (qres.next()) {
        int64_t snakid = qres.get<int64_t>("snak");
        qualifiers.push_back(getSnak(snakid));
    }

    // sources
    cppdb::result sres =
            (sql << "SELECT snak FROM source WHERE stmt = ?" << id);
    while (sres.next()) {
        int64_t snakid = sres.get<int64_t>("snak");
        qualifiers.push_back(getSnak(snakid));
    }

    return NewStatement(id, qid, getSnak(snak),
                     qualifiers, sources,
                     dataset, upload,
                     getApprovalState(state),
                     getLogEntries(id)
    );
}

Persistence::Persistence(cppdb::session &sql, bool managedTransactions)
        : sql(sql), managedTransactions(managedTransactions) { }

// helper method to distinguish between the case where we check duplicates and
// the case where we don't
int64_t Persistence::getOrAddSnak(const PropertyValue &pv, bool check_duplicates) {
    int64_t snakid = -1;
    if (check_duplicates) {
        snakid = getSnakID(pv);
    }
    if (snakid < 0) {
        snakid = addSnak(pv);
    }
    return snakid;
}

int64_t Persistence::addStatement(const Statement& st, bool check_duplicates) {
    if (!managedTransactions)
        sql.begin();

    int64_t snakid = getOrAddSnak(st.property_value(), check_duplicates);

    int64_t stmtid = (
            sql << "INSERT INTO statement(subject,mainsnak, dataset, upload) "
                   "VALUES (?,?,?,?)"
                << st.qid() << snakid
                << st.dataset() << st.upload() << cppdb::exec
    ).last_insert_id();

    for (const PropertyValue& pv : st.qualifiers()) {
        int64_t qualid = getOrAddSnak(pv, check_duplicates);

        sql << "INSERT INTO qualifier(stmt,snak) VALUES (?,?)"
            << stmtid << qualid << cppdb::exec;
    }

    for (const PropertyValue& pv : st.sources()) {
        int64_t qualid = getOrAddSnak(pv, check_duplicates);

        sql << "INSERT INTO source(stmt,snak) VALUES (?,?)"
            << stmtid << qualid << cppdb::exec;
    }

    if (!managedTransactions)
        sql.commit();

    return stmtid;
}

void Persistence::updateStatement(const Statement &st) {
    updateStatement(st.id(), st.approval_state());
}

void Persistence::updateStatement(int64_t id, ApprovalState state) {
    if (!managedTransactions)
        sql.begin();

    sql << "UPDATE statement SET state = ? WHERE id = ?"
        << getSQLState(state) << id << cppdb::exec;

    if (!managedTransactions)
        sql.commit();
}

void Persistence::addUserlog(const std::string &user, int64_t stmtid, ApprovalState state) {
    if (!managedTransactions)
        sql.begin();

    sql << "INSERT INTO userlog(user, stmt, state) VALUES (?, ?, ?)"
    << user << stmtid << getSQLState(state) << cppdb::exec;

    if (!managedTransactions)
        sql.commit();
}


Statement Persistence::getStatement(int64_t id) {
    if (!managedTransactions)
        sql.begin();

    cppdb::result res =(
            sql << "SELECT id, subject, mainsnak, state, dataset, upload "
                   "FROM statement WHERE id = ?"
                    << id << cppdb::row);

    if (!res.empty()) {
        Statement result = buildStatement(
                res.get<int64_t>("id"), res.get<std::string>("subject"),
                res.get<int64_t>("mainsnak"), res.get<std::string>("dataset"),
                res.get<int64_t>("upload"), res.get<int16_t>("state"));

        if (!managedTransactions)
            sql.commit();

        return result;
    } else {
        if (!managedTransactions)
            sql.commit();

        throw PersistenceException("statement not found");
    }
}

std::vector<Statement> Persistence::getStatementsByQID(
        const std::string &qid, ApprovalState state,
        const std::string &dataset) {
    if (!managedTransactions)
        sql.begin();

    std::vector<Statement> result;

    int16_t sqlState = getSQLState(state);
    cppdb::result res =(
            sql << "SELECT id, subject, mainsnak, state, dataset, upload "
                    "FROM statement WHERE subject = ?"
                    " AND (state = ? OR ?) AND (dataset = ? OR ?)"
                << qid
                << sqlState << (sqlState == -1)
                << dataset << (dataset == ""));


    while (res.next()) {
        result.push_back(buildStatement(
                res.get<int64_t>("id"), res.get<std::string>("subject"),
                res.get<int64_t>("mainsnak"), res.get<std::string>("dataset"),
                res.get<int64_t>("upload"), res.get<int16_t>("state")));
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


std::vector<Statement> Persistence::getAllStatements(
        int offset, int limit,
        ApprovalState state,
        const std::string& dataset,
        const std::string& property,
        const Value* value) {
    if (!managedTransactions)
        sql.begin();

    std::vector<Statement> result;

    std::string valueSelector = "";
    if (value != nullptr) {
        if (value->has_entity()) {
            valueSelector = "AND snak.svalue = ?";
        } else if (value->has_literal()) {
            valueSelector = "AND snak.svalue = ? AND snak.lang = ?";
        } else if (value->has_quantity()) {
            valueSelector = "AND snak.dvalue = ?";
        } else if (value->has_time()) {
            valueSelector = "AND snak.tvalue = ?";
        } else if (value->has_location()) {
            valueSelector = "AND snak.lat = ? AND snak.lng = ?";
        } else {
            throw PersistenceException("Value search for this type is not supported");
        }
    }

    int16_t sqlState = getSQLState(state);
    cppdb::statement statement = (
            sql << "SELECT statement.id AS sid, subject, mainsnak, state, dataset, upload "
                   "FROM statement INNER JOIN snak ON statement.mainsnak = snak.id "
                   "WHERE (statement.state = ? OR ?) AND (statement.dataset = ? OR ?) "
                   "AND (snak.property = ? OR ?) " + valueSelector + " "
                   "ORDER BY statement.id LIMIT ? OFFSET ?"
                << sqlState << (sqlState == -1)
                << dataset << (dataset == "")
                << property << (property == ""));

    if (value != nullptr) {
        if (value->has_entity()) {
            statement = (statement << value->entity().qid());
        } else if (value->has_literal()) {
            statement = (statement << value->literal().content() << value->literal().language());
        } else if (value->has_quantity()) {
            statement = (statement << std::stold(value->quantity().decimal()));
        } else if (value->has_time()) {
            statement = (statement << model::toSQLString(value->time()));
        } else if (value->has_location()) {
            statement = (statement << value->location().latitude() << value->location().longitude());
        }
    }

    cppdb::result res = (statement << limit << offset);

    while (res.next()) {
        result.push_back(buildStatement(
                res.get<int64_t>("sid"), res.get<std::string>("subject"),
                res.get<int64_t>("mainsnak"), res.get<std::string>("dataset"),
                res.get<int64_t>("upload"), res.get<int16_t>("state")));
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


std::vector<Statement> Persistence::getRandomStatements(
        int count, ApprovalState state) {
    if (!managedTransactions)
        sql.begin();

    std::vector<Statement> result;

    int16_t sqlState = getSQLState(state);
    std::string query;

    if(sql.engine() == "mysql") {
        query ="SELECT id, subject, mainsnak, state, dataset, upload "
                "FROM statement WHERE (state = ? OR ?) "
                "AND id >= RAND() * (SELECT max(id) FROM statement) "
                "ORDER BY id LIMIT ?";
    } else {
        query = "SELECT id, subject, mainsnak, state, dataset, upload "
                "FROM statement WHERE (state = ? OR ?) "
                "AND id >= abs(RANDOM()) % (SELECT max(id) FROM statement) "
                "ORDER BY id LIMIT ?";
    }

    cppdb::result res =(sql << query << sqlState << (sqlState == -1) << count);

    while (res.next()) {
        result.push_back(buildStatement(
                res.get<int64_t>("id"), res.get<std::string>("subject"),
                res.get<int64_t>("mainsnak"), res.get<std::string>("dataset"),
                res.get<int64_t>("upload"), res.get<int16_t>("state")));
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}

std::string Persistence::getRandomQID(ApprovalState state, const std::string &dataset) {
    if (!managedTransactions)
        sql.begin();

    int16_t sqlState = getSQLState(state);

    std::string query;
    if(sql.engine() == "mysql") {
        query = "SELECT subject "
                "FROM statement WHERE (state = ? OR ?) AND (dataset = ? OR ?) "
                "AND id >= RAND() * (SELECT max(id) FROM statement "
                "WHERE (state = ? OR ?) AND (dataset = ? OR ?)) "
                "ORDER BY id "
                "LIMIT 1";
    } else {
        query = "SELECT subject "
                "FROM statement WHERE (state = ? OR ?) AND (dataset = ? OR ?) "
                "AND id >= abs(RANDOM()) % (SELECT max(id) FROM statement "
                "WHERE (state = ? OR ?) AND (dataset = ? OR ?)) "
                "ORDER BY id "
                "LIMIT 1";
    }

    cppdb::result res =(
            sql << query
                << sqlState << (sqlState == -1) << dataset << (dataset == "")
                << sqlState << (sqlState == -1) << dataset << (dataset == "") << cppdb::row);


    if (!res.empty()) {
        std::string result = res.get<std::string>("subject");

        if (!managedTransactions)
            sql.commit();

        return result;
    } else {
        if (!managedTransactions)
            sql.commit();

        throw PersistenceException("no entity found");
    }
}


int64_t Persistence::countStatements(const std::string& dataset) {
    int64_t result = 0;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = (
            sql << "SELECT count(*) FROM statement "
                   "WHERE dataset = ? OR ?"
                << dataset << (dataset == "")
                << cppdb::row);

    if (!res.empty()) {
        result = res.get<int64_t>(0);
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


int64_t Persistence::countStatements(ApprovalState state, const std::string& dataset) {
    int64_t result = 0;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = (
            sql << "SELECT count(*) FROM statement "
                   "WHERE state = ? AND (dataset = ? OR ?)"
                << getSQLState(state) << dataset << (dataset == "")
                << cppdb::row);

    if (!res.empty()) {
        result = res.get<int64_t>(0);
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


int32_t Persistence::countUsers() {
    int32_t result = 0;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = (
            sql << "SELECT count(distinct(user)) FROM userlog"
    );

    if (!res.empty()) {
        result = res.get<int32_t>(0);
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


std::vector<std::pair<std::string, int64_t>> Persistence::getTopUsers(int32_t limit) {
    std::vector<std::pair < std::string, int64_t>> result;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = (
            sql << "SELECT user, count(id) AS activities FROM userlog "
                   "WHERE state != 5 AND state != 6 "
                   "GROUP BY user ORDER BY activities DESC LIMIT ?"
                << limit
    );

    while(res.next()) {
        std::string user = res.get<std::string>(0);
        int64_t activities = res.get<int64_t>(1);
        result.push_back(std::pair<std::string, int64_t>(user, activities));
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


void Persistence::markDuplicates(int64_t start_id) {

    // always force batch operation in one transaction
    bool hadManagedTransactions = managedTransactions;
    if (!managedTransactions) {
        sql.begin();
        managedTransactions = true;
    }

    std::cout << "DEDUPLICATE: start" << std::endl;

    // list all entities with unapproved statements
    cppdb::result r_entities =(
            sql << "SELECT DISTINCT subject "
                   "FROM statement WHERE state = 0 AND id > ?"
                << start_id);

    std::cout << "DEDUPLICATE: entities selected" << std::endl;

    int64_t stmts_processed = 0, stmts_marked = 0;
    std::string qid;
    std::vector<Statement> statements;
    while (r_entities.next()) {
        qid = r_entities.get<std::string>("subject");
        statements = getStatementsByQID(qid, ApprovalState::ANY);

        if (statements.size() > 1) {
            for (auto it1 = statements.begin(); it1 != statements.end(); it1++) {
                Statement s1 = *it1;
                for (auto it2 = it1 + 1; it2 != statements.end(); it2++) {
                    Statement s2 = *it2;
                    if (s1 == s2 && s2.approval_state() == ApprovalState::UNAPPROVED) {
                        s2.set_approval_state(ApprovalState::DUPLICATE);
                        updateStatement(s2);
                        stmts_marked++;
                    }
                    stmts_processed++;

                    if (stmts_processed % 10000 == 0) {
                        std::cout << "DEDUPLICATE: processed " << stmts_processed << " statements (" << stmts_marked << " duplicates)" << std::endl;
                    }
                }
            }
        }
    }
    std::cout << "DEDUPLICATE: processed " << stmts_processed << " statements (" << stmts_marked << " duplicates)" << std::endl;

    if (!hadManagedTransactions) {
        sql.commit();
        managedTransactions = hadManagedTransactions;
    }
}


void Persistence::deleteStatements(ApprovalState state) {
    if (!managedTransactions)
        sql.begin();

    sql << "DELETE FROM statement WHERE state = ?"
        << getSQLState(state) << cppdb::exec;

    if (!managedTransactions)
        sql.commit();
}


std::vector<std::string> Persistence::getDatasets() {
    if (!managedTransactions)
        sql.begin();

    std::vector<std::string> result;

    cppdb::result res =(
            sql << "SELECT DISTINCT dataset FROM statement");


    while (res.next()) {
        result.push_back(res.get<std::string>("dataset"));
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}


const char *PersistenceException::what() const noexcept {
    return message.c_str();
}


}  // namespace primarysources
}  // namespace wikidata

