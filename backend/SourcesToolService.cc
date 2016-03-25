// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolService.h"

#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>

#include "SerializerTSV.h"
#include "SerializerJSON.h"
#include "Persistence.h"
#include "Membuf.h"
#include "Version.h"
#include "Parser.h"
#include "TimeLogger.h"
#include "MemStat.h"

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

    dispatcher().assign("/statements/all",
                &SourcesToolService::getAllStatements, this);
    mapper().assign("stmt_all", "/statements/all");

    dispatcher().assign("/import",
            &SourcesToolService::importStatements, this);
    mapper().assign("import", "/import");

    dispatcher().assign("/delete",
                        &SourcesToolService::deleteStatements, this);
    mapper().assign("delete", "/delete");

    dispatcher().assign("/datasets/all",
                            &SourcesToolService::getAllDatasets, this);
        mapper().assign("datasets_all", "/datasets/all");

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
    wikidata::primarysources::TimeLogger timer("GET /entities/" + qid);

    try {
        // By default only return unapproved statements.
        ApprovalState state = UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        std::vector<Statement> statements = backend
                .getStatementsByQID(cache(), qid, state, request().get("dataset"));

        addCORSHeaders();
        addVersionHeaders();

        if (statements.size() > 0) {
            serializeStatements(statements);
        } else {
            response().status(404, "no statements found for entity " + qid);
        }

        reqGetEntityCount++;
    } catch(InvalidApprovalState const &e) {
        response().status(400, "Bad Request: invalid state parameter");
    }
}

void SourcesToolService::getRandomEntity() {
    wikidata::primarysources::TimeLogger timer("GET /entities/any");

    addCORSHeaders();
    addVersionHeaders();

    try {
        // By default only return unapproved statements.
        ApprovalState state = UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        std::vector<Statement> statements =
                backend.getStatementsByRandomQID(cache(), state, request().get("dataset"));
        serializeStatements(statements);
    } catch(InvalidApprovalState const &e) {
        response().status(400, "Bad Request: invalid state parameter");
    } catch(PersistenceException const &e) {
        response().status(404, "no random unapproved entity found");
    }

    reqGetRandomCount++;
}

void SourcesToolService::approveStatement(int64_t stid) {
    wikidata::primarysources::TimeLogger timer("POST /statements/" + std::to_string(stid));

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
}

void SourcesToolService::getStatement(int64_t stid) {
    wikidata::primarysources::TimeLogger timer("GET /statements/" + std::to_string(stid));

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
}

void SourcesToolService::getRandomStatements() {
    wikidata::primarysources::TimeLogger timer("GET /statements/any");

    addCORSHeaders();
    addVersionHeaders();

    try {
        int count = 10;
        if (request().get("count") != "") {
            count = std::stoi(request().get("count"));
        }

        // By default only return unapproved statements.
        ApprovalState state = UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        serializeStatements(backend.getRandomStatements(cache(), count, state));
    } catch(InvalidApprovalState const &e) {
        response().status(400, "Bad Request: invalid state parameter");
    }
}

void SourcesToolService::getAllStatements() {
    wikidata::primarysources::TimeLogger timer("GET /statements/all");

    addCORSHeaders();
    addVersionHeaders();

    try {
        int offset = 0;
        if (request().get("offset") != "") {
            offset = std::stoi(request().get("offset"));
        }

        int limit = 10;
        if (request().get("limit") != "") {
            limit = std::min(std::stoi(request().get("limit")), 1000);
        }

        // By default return only unapproved statements.
        ApprovalState state = UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        std::shared_ptr<Value> value;
        if (request().get("value") != "") {
            value = std::make_shared<Value>(Parser::parseValue(request().get("value")));
        }

        std::vector<Statement> statements =
                backend.getAllStatements(cache(), offset, limit,
                                         state,
                                         request().get("dataset"),
                                         request().get("property"),
                                         value);
        if (statements.size() > 0) {
            serializeStatements(statements);
        } else {
            response().status(404, "no statements found");
        }
    } catch(InvalidApprovalState const &e) {
        response().status(400, "Bad Request: invalid or missing state parameter");
    }
}


void SourcesToolService::getStatus() {
    wikidata::primarysources::TimeLogger timer("GET /status");

    addCORSHeaders();
    addVersionHeaders();

    cppcms::json::value result;

    std::string dataset = request().get("dataset");
    Status status = backend.getStatus(cache(), dataset);

    // show dataset-specific statements count, defaulting to all datasets
    if (dataset != "") {
        result["dataset"] = dataset;
    } else {
        result["dataset"] = "all";
    }

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

    wikidata::primarysources::MemStat meminfo;
    result["system"]["shared_mem"] = meminfo.getSharedMem();
    result["system"]["private_mem"] = meminfo.getPrivateMem();
    result["system"]["rss"] = meminfo.getRSS();

    // request statistics
    result["requests"]["getentity"] = reqGetEntityCount;
    result["requests"]["getrandom"] = reqGetRandomCount;
    result["requests"]["getstatement"] = reqGetStatementCount;
    result["requests"]["updatestatement"] = reqUpdateStatementCount;
    result["requests"]["getstatus"] = reqStatusCount;

    response().content_type("application/json");
    result.save(response().out(), cppcms::json::readable);

    reqStatusCount++;
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

        std::string dataset = request().get("dataset");
        if (dataset == "") {
            response().status(400, "Missing argument: dataset");
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

        {
            wikidata::primarysources::TimeLogger timer("POST /import");

            // wrap raw post data into a memory stream
            Membuf body(request().raw_post_data());
            std::istream in(&body);

            // import statements
            int64_t count = backend.importStatements(
                    cache(), in, dataset, gzip, dedup);
    
            cppcms::json::value result;
            result["count"] = count;
            result["time"] = timer.Elapsed().count();
            result["dataset"] = dataset;
    
            response().content_type("application/json");
            result.save(response().out(), cppcms::json::readable);
        }
    } else {
        response().status(405, "Method not allowed");
        response().set_header("Allow", "POST");
    }
}

void SourcesToolService::getAllDatasets() {
	addCORSHeaders();
	addVersionHeaders();

	std::vector<std::string> datasets = backend.getDatasets(cache());

	cppcms::json::value result;
	for(std::string id : datasets) {
        result[id]["id"] = id;
	}

    response().content_type("application/json");
    result.save(response().out(), cppcms::json::readable);
}

void SourcesToolService::deleteStatements() {
    addVersionHeaders();

    if (request().request_method() == "POST") {

        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            response().status(401, "Invalid authorization token");
            return;
        }

        wikidata::primarysources::TimeLogger timer("POST /delete");

        // check if statement exists and update it with new state
        try {
            backend.deleteStatements(cache(), stateFromString(request().get("state")));
        } catch(PersistenceException const &e) {
            response().status(404, "Could not delete statements");
        } catch(InvalidApprovalState const &e) {
            response().status(400, "Bad Request: invalid or missing state parameter");
        }
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
    wikidata::primarysources::TimeLogger timer("GET /dashboard/activitylog");

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
}
