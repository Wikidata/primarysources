// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include <sys/types.h>
#include "Persistence.h"
#include "Statement.h"

inline ApprovalState getApprovalState(int16_t state) {
    switch (state) {
        case 0: return UNAPPROVED;
        case 1: return APPROVED;
        case 2: return OTHERSOURCE;
        case 3: return WRONG;
        case 4: return SKIPPED;
        case 5: return DUPLICATE;
        case 6: return BLACKLISTED;
        default: return UNAPPROVED;
    }
}

inline int16_t getSQLState(ApprovalState state) {
    switch (state) {
        case UNAPPROVED:  return 0;
        case APPROVED:    return 1;
        case OTHERSOURCE: return 2;
        case WRONG:       return 3;
        case SKIPPED:     return 4;
        case DUPLICATE:   return 5;
        case BLACKLISTED: return 6;
    }
}


int64_t Persistence::addSnak(const PropertyValue &pv) {
    switch (pv.getValue().getType()) {
        case ITEM:
            return (sql << "INSERT INTO snak(property,svalue,vtype) "
                           "VALUES (?,?,'item')"
                        << pv.getProperty() << pv.getValue().getString()
                        << cppdb::exec).last_insert_id();
        case STRING:
            return (sql << "INSERT INTO snak(property,svalue,lang,vtype) "
                           "VALUES (?,?,?,'string')"
                        << pv.getProperty() << pv.getValue().getString()
                        << pv.getValue().getLanguage() << cppdb::exec)
                    .last_insert_id();
        case QUANTITY:
            return (sql << "INSERT INTO snak(property,dvalue,vtype) "
                           "VALUES (?,?,'quantity')"
                        << pv.getProperty()
                        << static_cast<long double>(pv.getValue().getQuantity())
                        << cppdb::exec).last_insert_id();
        case TIME:
            return (sql << "INSERT INTO snak(property,tvalue,precision,vtype) "
                           "VALUES (?,?,?,'time')"
                        << pv.getProperty() << pv.getValue().getTime()
                        << pv.getValue().getPrecision() << cppdb::exec)
                    .last_insert_id();
        case LOCATION:
            return (sql << "INSERT INTO snak(property,lat,lng,vtype) "
                           "VALUES (?,?,?,'location')"
                        << pv.getProperty() << pv.getValue().getLocation().first
                        << pv.getValue().getLocation().second << cppdb::exec)
                    .last_insert_id();
    }

    return -1;
}

int64_t Persistence::getSnakID(const PropertyValue &pv) {
    cppdb::result r;
    switch (pv.getValue().getType()) {
        case ITEM:
            r = (sql << "SELECT id FROM snak "
                        "WHERE property=? AND svalue=? AND vtype='item'"
                     << pv.getProperty() << pv.getValue().getString()
                     << cppdb::row);
            break;
        case STRING:
            r = (sql << "SELECT id FROM snak "
                        "WHERE property=? AND svalue=? AND lang=? "
                        "AND vtype='string'"
                    << pv.getProperty() << pv.getValue().getString()
                    << pv.getValue().getLanguage() << cppdb::row);
            break;
        case QUANTITY:
            r= (sql << "SELECT id FROM snak "
                       "WHERE property=? AND dvalue=? AND vtype='quantity'"
                    << pv.getProperty()
                    << static_cast<long double>(pv.getValue().getQuantity())
                    << cppdb::row);
            break;
        case TIME:
            r = (sql << "SELECT id FROM snak "
                        "WHERE property=? AND tvalue=? AND precision=? "
                        "AND vtype='time'"
                    << pv.getProperty() << pv.getValue().getTime()
                    << pv.getValue().getPrecision() << cppdb::row);
            break;
        case LOCATION:
            r = (sql << "SELECT id FROM snak "
                        "WHERE property=? AND lat=? AND lng=? "
                        "AND vtype='location' "
                    << pv.getProperty() << pv.getValue().getLocation().first
                    << pv.getValue().getLocation().second << cppdb::row);
            break;
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
                   "       lat, lng, precision, lang, vtype "
                   "FROM snak WHERE id = ?"
                << snakid << cppdb::row);

    if (!res.empty()) {
        std::string vtype = res.get<std::string>("vtype");
        std::string prop  = res.get<std::string>("property");
        if (vtype == "item") {
            std::string svalue = res.get<std::string>("svalue");
            return PropertyValue(prop, Value(svalue));
        } else if (vtype == "string") {
            std::string svalue = res.get<std::string>("svalue");
            std::string lang = res.get<std::string>("lang");
            return PropertyValue(prop, Value(svalue, lang));
        } else if (vtype == "time") {
            std::tm tvalue = res.get<std::tm>("tvalue");
            int precision = res.get<int>("precision");
            return PropertyValue(prop, Value(tvalue, precision));
        } else if (vtype == "location") {
            double lat = res.get<double>("lat");
            double lng = res.get<double>("lng");
            return PropertyValue(prop, Value(lat, lng));
        } else if (vtype == "quantity") {
            std::string dvalue = res.get<std::string>("dvalue");
            return PropertyValue(prop, Value(decimal_t(dvalue)));
        } else {
            // no result found
            throw PersistenceException("snak has unknown type");
        }

    } else {
        // no result found
        throw PersistenceException("snak not found");
    }
}

Statement Persistence::buildStatement(int64_t id, const std::string& qid,
                                      int64_t snak, const std::string& dataset,
                                      int64_t upload, int16_t state) {
    Statement::extensions_t qualifiers, sources;

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

    return Statement(id, qid, getSnak(snak),
                     qualifiers, sources,
                     dataset, upload, getApprovalState(state));
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

    int64_t snakid = getOrAddSnak(st.getPropertyValue(), check_duplicates);

    int64_t stmtid = (
            sql << "INSERT INTO statement(subject,mainsnak, dataset, upload) "
                   "VALUES (?,?,?,?)"
                << st.getQID() << snakid
                << st.getDataset() << st.getUpload() << cppdb::exec
    ).last_insert_id();

    for (const PropertyValue& pv : st.getQualifiers()) {
        int64_t qualid = getOrAddSnak(pv, check_duplicates);

        sql << "INSERT INTO qualifier(stmt,snak) VALUES (?,?)"
            << stmtid << qualid << cppdb::exec;
    }

    for (const PropertyValue& pv : st.getSources()) {
        int64_t qualid = getOrAddSnak(pv, check_duplicates);

        sql << "INSERT INTO source(stmt,snak) VALUES (?,?)"
            << stmtid << qualid << cppdb::exec;
    }

    if (!managedTransactions)
        sql.commit();

    return stmtid;
}

void Persistence::updateStatement(const Statement &st) {
    updateStatement(st.getID(), st.getApprovalState());
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
        const std::string &qid, bool unapprovedOnly, std::string dataset) {
    if (!managedTransactions)
        sql.begin();

    std::vector<Statement> result;

    cppdb::result res =(
            sql << "SELECT id, subject, mainsnak, state, dataset, upload "
                    "FROM statement WHERE subject = ?"
                    " AND (state = 0 OR ?) AND (dataset = ? OR ?)"
                    << qid << !unapprovedOnly << dataset << (dataset == ""));


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


std::vector<Statement> Persistence::getRandomStatements(
        int count, bool unapprovedOnly) {
    if (!managedTransactions)
        sql.begin();

    std::vector<Statement> result;

    cppdb::result res =(
            sql << "SELECT id, subject, mainsnak, state, dataset, upload "
                    "FROM statement WHERE (state = 0 OR ?) "
                    "AND id >= abs(random()) % (SELECT max(id) FROM statement) "
                    "ORDER BY id LIMIT ?"
                    << !unapprovedOnly << count);


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

std::string Persistence::getRandomQID(bool unapprovedOnly, std::string dataset) {
    if (!managedTransactions)
        sql.begin();

    cppdb::result res =(
            sql << "SELECT subject "
                    "FROM statement WHERE (state = 0 OR ?) AND (dataset = ? OR ?) "
                    "AND id >= abs(random()) % (SELECT max(id) FROM statement "
                    "WHERE (state = 0 OR ?) AND (dataset = ? OR ?)) "
                    "ORDER BY id "
                    "LIMIT 1"
                    << !unapprovedOnly << dataset << (dataset == "")
                    << !unapprovedOnly  << dataset << (dataset == "") << cppdb::row);


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


int64_t Persistence::countStatements() {
    int64_t result = 0;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = sql << "SELECT count(*) FROM statement" << cppdb::row;

    if (!res.empty()) {
        result = res.get<int64_t>(0);
    }

    if (!managedTransactions)
        sql.commit();

    return result;
}

int64_t Persistence::countStatements(ApprovalState state) {
    int64_t result = 0;

    if (!managedTransactions)
        sql.begin();

    cppdb::result res = (
            sql << "SELECT count(*) FROM statement WHERE state = ?"
                << getSQLState(state) << cppdb::row);

    if (!res.empty()) {
        result = res.get<int64_t>(0);
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
        statements = getStatementsByQID(qid, false);

        if (statements.size() > 1) {
            for (auto it1 = statements.begin(); it1 != statements.end(); it1++) {
                Statement s1 = *it1;
                for (auto it2 = it1 + 1; it2 != statements.end(); it2++) {
                    Statement s2 = *it2;
                    if (s1 == s2 && s2.getApprovalState() == UNAPPROVED) {
                        s2.setApprovalState(DUPLICATE);
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

