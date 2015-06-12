// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolService.h"

#include <booster/log.h>
#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <fstream>

#include "SerializerTSV.h"
#include "SerializerJSON.h"
#include "Persistence.h"
#include "Membuf.h"
#include "Version.h"

#define TIMING(message, begin, end) \
    BOOSTER_NOTICE("sourcestool") \
        << request().remote_addr() << ": " \
        << (message) << " time: " \
        << 1000 * (static_cast<double>(end - begin) / CLOCKS_PER_SEC) \
        << "ms" << std::endl;

// initialise static counters for status reports
time_t SourcesToolService::startupTime = std::time(NULL);
int64_t SourcesToolService::reqGetEntityCount = 0;
int64_t SourcesToolService::reqGetRandomCount = 0;
int64_t SourcesToolService::reqGetStatementCount = 0;
int64_t SourcesToolService::reqUpdateStatementCount = 0;
int64_t SourcesToolService::reqStatusCount = 0;

// format a time_t using ISO8601 GMT time
inline std::string formatGMT(time_t* time) {
    char result[128];
    std::strftime(result, 128, "%Y-%m-%dT%H:%M:%SZ", gmtime(time));
    return std::string(result);
}

// represent memory statistics (used for status)
struct memstat {
    double rss, shared_mem, private_mem;
};

// read memory statistics from /proc (used for status)
inline memstat getMemStat() {
    int tSize = 0, resident = 0, share = 0;
    std::ifstream buffer("/proc/self/statm");
    buffer >> tSize >> resident >> share;
    buffer.close();

    struct memstat result;

    long page_size_kb = sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
    result.rss = resident * page_size_kb;
    result.shared_mem = share * page_size_kb;
    result.private_mem = result.rss - result.shared_mem;

    return result;
}

// initialise service mappings from URLs to methods
SourcesToolService::SourcesToolService(cppcms::service &srv)
        : cppcms::application(srv), backend(settings()["database"]) {

    dispatcher().assign("/entities/(Q\\d+)",
            &SourcesToolService::getEntityByQID, this, 1);
    mapper().assign("entity_by_qid", "/entities/{1}");

    // map request to random entity selector
    dispatcher().assign("/entities/any",
            &SourcesToolService::getRandomEntity, this);
    mapper().assign("entity_by_topic_user", "/entities/any");

    // map GET and POST requests to /entities/<QID> to the respective handlers
    // we use a helper function to distinguish both cases, since cppcms
    // currently does not really support REST
    dispatcher().assign("/statements/(\\d+)",
            &SourcesToolService::handleGetPostStatement, this, 1);
    mapper().assign("stmt_by_id", "/statements/{1}");

    dispatcher().assign("/statements/any",
            &SourcesToolService::getRandomStatements, this);
    mapper().assign("stmt_by_random", "/statements/any");

    dispatcher().assign("/import",
            &SourcesToolService::importStatements, this);
    mapper().assign("import", "/import");

    dispatcher().assign("/delete",
                        &SourcesToolService::deleteStatements, this);
    mapper().assign("delete", "/delete");

    dispatcher().assign("/status",
                        &SourcesToolService::getStatus, this);
    mapper().assign("status", "/status");

    dispatcher().assign("/dashboard/activitylog",
                        &SourcesToolService::getActivityLog, this);
    mapper().assign("activitylog", "/dashboard/activitylog");
}


void SourcesToolService::handleGetPostStatement(std::string stid) {
    if (request().request_method() == "POST") {
        approveStatement(std::stoll(stid));
    } else {
        getStatement(std::stoll(stid));
    }
}

void SourcesToolService::getEntityByQID(std::string qid) {
    clock_t begin = std::clock();

    std::vector<Statement> statements = backend.getStatementsByQID(cache(), qid, true);

    addCORSHeaders();
    addVersionHeaders();

    if (statements.size() > 0) {
        serializeStatements(statements);
    } else {
        response().status(404, "no statements found for entity "+qid);
    }

    reqGetEntityCount++;

    clock_t end = std::clock();
    TIMING("GET /entities/" + qid, begin, end);
}

void SourcesToolService::getRandomEntity() {
    clock_t begin = std::clock();

    addCORSHeaders();
    addVersionHeaders();

    try {
        std::vector<Statement> statements = backend.getStatementsByRandomQID(cache(), true);
        serializeStatements(statements);
    } catch(PersistenceException const &e) {
        response().status(404, "no random unapproved entity found");
    }

    reqGetRandomCount++;

    clock_t end = std::clock();
    TIMING("GET /entities/any", begin, end);
}

void SourcesToolService::approveStatement(int64_t stid) {
    clock_t begin = std::clock();

    ApprovalState state;

    addCORSHeaders();
    addVersionHeaders();

    // return 403 forbidden when there is no user given or the username is too
    // long for the database
    if (request().get("user") == "" || request().get("user").length() > 64) {
        response().status(403, "Forbidden: invalid or missing user");
        return;
    }

    // check if statement exists and update it with new state
    try {
        backend.updateStatement(cache(), stid, stateFromString(request().get("state")), request().get("user"));
    } catch(PersistenceException const &e) {
        response().status(404, "Statement not found");
    } catch(InvalidApprovalState const &e) {
        response().status(400, "Bad Request: invalid or missing state parameter");
    }

    reqUpdateStatementCount++;

    clock_t end = std::clock();
    TIMING("POST /statements" + std::to_string(stid), begin, end);
}

void SourcesToolService::getStatement(int64_t stid) {
    clock_t begin = std::clock();

    addCORSHeaders();
    addVersionHeaders();

    // query for statement, wrap it in a vector and return it
    try {
        std::vector<Statement> statements = { backend.getStatementByID(cache(), stid) };
        serializeStatements(statements);
    } catch(PersistenceException const &e) {
        std::cerr << "error: " << e.what() << std::endl;
        response().status(404, "Statement not found");
    }

    reqGetStatementCount++;

    clock_t end = std::clock();
    TIMING("GET /statements" + std::to_string(stid), begin, end);
}

void SourcesToolService::getRandomStatements() {
    clock_t begin = std::clock();

    int count = 10;
    if (request().get("count") != "") {
        count = std::stoi(request().get("count"));
    }

    addCORSHeaders();
    addVersionHeaders();

    serializeStatements(backend.getRandomStatements(cache(), count, true));

    clock_t end = std::clock();
    TIMING("GET /statements/any", begin, end);
}


void SourcesToolService::getStatus() {
    clock_t begin = std::clock();

    addCORSHeaders();
    addVersionHeaders();

    cppcms::json::value result;

    Status status = backend.getStatus(cache());

    result["statements"]["total"] = status.getStatements();
    result["statements"]["approved"] = status.getApproved();
    result["statements"]["unapproved"] = status.getUnapproved();
    result["statements"]["blacklisted"] = status.getBlacklisted();
    result["statements"]["duplicate"] = status.getDuplicate();
    result["statements"]["wrong"] = status.getWrong();

    cppcms::json::array topusers;
    for (auto entry : status.getTopUsers()) {
        cppcms::json::value v;
        v["name"] = entry.first;
        v["activities"] = entry.second;
        topusers.push_back(v);
    }
    result["topusers"] = topusers;

    // system information
    result["system"]["startup"] = formatGMT(&SourcesToolService::startupTime);
    result["system"]["version"] = std::string(GIT_SHA1);
    result["system"]["cache_hits"] = backend.getCacheHits();
    result["system"]["cache_misses"] = backend.getCacheMisses();

    struct memstat meminfo = getMemStat();
    result["system"]["shared_mem"] = meminfo.shared_mem;
    result["system"]["private_mem"] = meminfo.private_mem;
    result["system"]["rss"] = meminfo.rss;

    // request statistics
    result["requests"]["getentity"] = reqGetEntityCount;
    result["requests"]["getrandom"] = reqGetRandomCount;
    result["requests"]["getstatement"] = reqGetStatementCount;
    result["requests"]["updatestatement"] = reqUpdateStatementCount;
    result["requests"]["getstatus"] = reqStatusCount;

    response().content_type("application/json");
    result.save(response().out(), cppcms::json::readable);

    reqStatusCount++;

    clock_t end = std::clock();
    TIMING("GET /status", begin, end);
}

void SourcesToolService::serializeStatements(const std::vector<Statement> &statements) {
    if(request().http_accept() == "text/vnd.wikidata+tsv"
            || request().http_accept() == "text/tsv") {
        response().content_type("text/vnd.wikidata+tsv");

        Serializer::writeTSV(statements.cbegin(), statements.cend(), &response().out());
    } else if(request().http_accept() == "application/vnd.wikidata+json") {
        response().content_type("application/vnd.wikidata+json");

        Serializer::writeWikidataJSON(statements.cbegin(), statements.cend(), &response().out());
    } else {
        response().content_type("application/vnd.wikidata.envelope+json");

        Serializer::writeEnvelopeJSON(statements.cbegin(), statements.cend(), &response().out());
    }
}

void SourcesToolService::importStatements() {
    addVersionHeaders();

    if (request().request_method() == "POST") {
        clock_t begin = std::clock();

        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            response().status(401, "Invalid authorization token");
            return;
        }

        // check if content is gzipped
        bool gzip = false;
        if (request().get("gzip") == "true") {
            gzip = true;
        }

        bool dedup = true;
        if (request().get("deduplicate") == "false") {
            dedup = false;
        }

        // wrap raw post data into a memory stream
        Membuf body(request().raw_post_data());
        std::istream in(&body);

        // import statements
        int64_t count = backend.importStatements(cache(), in, gzip, dedup);

        clock_t end = std::clock();

        cppcms::json::value result;
        result["count"] = count;
        result["time"] = 1000 * (static_cast<double>(end - begin) / CLOCKS_PER_SEC);

        response().content_type("application/json");
        result.save(response().out(), cppcms::json::readable);

        TIMING("POST /import", begin, end);
    } else {
        response().status(405, "Method not allowed");
        response().set_header("Allow", "POST");
    }
}

void SourcesToolService::deleteStatements() {
    addVersionHeaders();

    if (request().request_method() == "POST") {

        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            response().status(401, "Invalid authorization token");
            return;
        }

        clock_t begin = std::clock();

        // check if statement exists and update it with new state
        try {
            backend.deleteStatements(cache(), stateFromString(request().get("state")));
        } catch(PersistenceException const &e) {
            response().status(404, "Could not delete statements");
        } catch(InvalidApprovalState const &e) {
            response().status(400, "Bad Request: invalid or missing state parameter");
        }

        clock_t end = std::clock();

        TIMING("POST /delete", begin, end);
    } else {
        response().status(405, "Method not allowed");
        response().set_header("Allow", "POST");
    }
}

void SourcesToolService::addCORSHeaders() {
    response().set_header("Access-Control-Allow-Origin", "*");
}

void SourcesToolService::addVersionHeaders() {
    response().set_header("X-Powered-By", std::string("Wikidata Sources Tool/") + GIT_SHA1);
}


void SourcesToolService::getActivityLog() {
    clock_t begin = std::clock();

    addCORSHeaders();
    addVersionHeaders();

    cppcms::json::value result;

    Dashboard::ActivityLog activities = backend.getActivityLog(cache());

    std::vector<std::string> users(activities.getUsers().begin(), activities.getUsers().end());

    result["users"] = users;
    for(int i=0; i< activities.getActivities().size(); i++) {
        Dashboard::ActivityEntry entry = activities.getActivities()[i];

        if (entry.date != "") {
            result["approved"][i][0] = entry.date;
            for (int j = 0; j < users.size(); j++) {
                if (entry.approved.find(users[j]) != entry.approved.end()) {
                    result["approved"][i][j + 1] = entry.approved[users[j]];
                } else {
                    result["approved"][i][j + 1] = 0;
                }
            }

            result["rejected"][i][0] = entry.date;
            for (int j = 0; j < users.size(); j++) {
                if (entry.rejected.find(users[j]) != entry.rejected.end()) {
                    result["rejected"][i][j + 1] = entry.rejected[users[j]];
                } else {
                    result["rejected"][i][j + 1] = 0;
                }
            }
        }

    }


    response().content_type("application/json");
    result.save(response().out(), cppcms::json::readable);

    reqStatusCount++;

    clock_t end = std::clock();
    TIMING("GET /dashboard/activitylog", begin, end);
}
