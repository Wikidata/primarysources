// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "SourcesToolService.h"

#include <cppcms/service.h>
#include <cppcms/http_response.h>
#include <cppcms/http_request.h>
#include <cppcms/url_dispatcher.h>
#include <cppcms/url_mapper.h>
#include <status/SystemStatus.h>
#include <model/status.pb.h>
#include <glog/logging.h>
#include <util/HttpStatus.h>
#include <util/json/json.h>

#include "serializer/SerializerTSV.h"
#include "serializer/SerializerJSON.h"
#include "persistence/Persistence.h"
#include "util/Membuf.h"
#include "parser/Parser.h"
#include "util/TimeLogger.h"
#include "util/Retry.h"

using wikidata::primarysources::model::ApprovalState;
using wikidata::primarysources::model::PropertyValue;
using wikidata::primarysources::model::Statement;
using wikidata::primarysources::model::Value;

using wikidata::primarysources::model::InvalidApprovalState;

using wikidata::primarysources::model::stateFromString;


namespace wikidata {
namespace primarysources {

// initialise service mappings from URLs to methods
SourcesToolService::SourcesToolService(cppcms::service &srv)
        : cppcms::application(srv), backend(settings()) {

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

    dispatcher().assign("/cache/clear",
                        &SourcesToolService::clearCache, this);
    mapper().assign("cache_clear", "/cache/clear");

    dispatcher().assign("/cache/update",
                        &SourcesToolService::updateCache, this);
    mapper().assign("cache_update", "/cache/update");
}


void SourcesToolService::handleGetPostStatement(std::string stid) {
    if (request().request_method() == "POST") {
        approveStatement(std::stoll(stid));
    } else {
        getStatement(std::stoll(stid));
    }
}

void SourcesToolService::clearCache() {
    TimeLogger timer("POST /cache/clear");

    addCORSHeaders();
    addVersionHeaders();

    if (request().request_method() == "POST") {
        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            RESPONSE(UNAUTHORIZED) << "Invalid authorization token";
            return;
        }

        backend.ClearCachedEntities(cache());
    }
}

void SourcesToolService::updateCache() {
    TimeLogger timer("POST /cache/update");

    addCORSHeaders();
    addVersionHeaders();

    if (request().request_method() == "POST") {
        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            RESPONSE(UNAUTHORIZED) << "Invalid authorization token";
            return;
        }

        backend.UpdateCachedEntities(cache());
    }
}

void SourcesToolService::getEntityByQID(std::string qid) {
    TimeLogger timer("GET /entities/" + qid);

    try {
        // By default only return unapproved statements.
        ApprovalState state = ApprovalState::UNAPPROVED;
        if (request().get("state") != "") {
            state = model::stateFromString(request().get("state"));
        }

        RETRY({
                  std::vector<Statement> statements = backend
                          .getStatementsByQID(cache(), qid, state, request().get("dataset"));

                  addCORSHeaders();
                  addVersionHeaders();

                  if (statements.size() > 0) {
                      serializeStatements(statements);
                  } else {
                      RESPONSE(NOT_FOUND) << "No statements found for entity " << qid;
                  }

                  backend.StatusService().AddGetEntityRequest();
              }, 3, cppdb::cppdb_error);
    } catch(InvalidApprovalState const &e) {
        RESPONSE(BAD_REQUEST) << "Invalid state parameter '" << request().get("state") << "'";
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    }
}

void SourcesToolService::getRandomEntity() {
    TimeLogger timer("GET /entities/any");

    addCORSHeaders();
    addVersionHeaders();

    try {
        // By default only return unapproved statements.
        ApprovalState state = ApprovalState::UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        RETRY({
                  std::vector<Statement> statements =
                          backend.getStatementsByRandomQID(cache(), state, request().get("dataset"));
                  serializeStatements(statements);
              }, 3, cppdb::cppdb_error);
    } catch(InvalidApprovalState const &e) {
        RESPONSE(BAD_REQUEST) << "invalid state parameter";
    } catch(PersistenceException const &e) {
        RESPONSE(NOT_FOUND) << "no random unapproved entity found";
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    }

    backend.StatusService().AddGetRandomRequest();
}

void SourcesToolService::approveStatement(int64_t stid) {
    TimeLogger timer("POST /statements/" + std::to_string(stid));

    addCORSHeaders();
    addVersionHeaders();

    // return 403 forbidden when there is no user given or the username is too
    // long for the database
    if (request().get("user") == "" || request().get("user").length() > 64) {
        RESPONSE(FORBIDDEN) << "invalid or missing user";
        return;
    }

    // check if statement exists and update it with new state
    try {
        RETRY({
                  backend.updateStatement(cache(), stid, stateFromString(request().get("state")), request().get("user"));
              }, 3, cppdb::cppdb_error);
    } catch(PersistenceException const &e) {
        RESPONSE(NOT_FOUND) << "Statement " << stid  << " not found";
    } catch(InvalidApprovalState const &e) {
        RESPONSE(BAD_REQUEST) << "Invalid or missing state parameter";
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    }

    backend.StatusService().AddUpdateStatementRequest();
}

void SourcesToolService::getStatement(int64_t stid) {
    TimeLogger timer("GET /statements/" + std::to_string(stid));

    addCORSHeaders();
    addVersionHeaders();

    // query for statement, wrap it in a vector and return it
    try {
        RETRY({
                  std::vector<Statement> statements = { backend.getStatementByID(cache(), stid) };
                  serializeStatements(statements);
              }, 3, cppdb::cppdb_error);
    } catch(PersistenceException const &e) {
        RESPONSE(NOT_FOUND) << "Statement " << stid  << " not found";
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    }

    backend.StatusService().AddGetStatementRequest();
}

void SourcesToolService::getRandomStatements() {
    TimeLogger timer("GET /statements/any");

    addCORSHeaders();
    addVersionHeaders();

    try {
        int count = 10;
        if (request().get("count") != "") {
            count = std::stoi(request().get("count"));
        }

        // By default only return unapproved statements.
        ApprovalState state = ApprovalState::UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        RETRY({
                  serializeStatements(backend.getRandomStatements(cache(), count, state));
              }, 3, cppdb::cppdb_error);
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    } catch(InvalidApprovalState const &e) {
        RESPONSE(BAD_REQUEST) << "Invalid or missing state parameter";
    }
}

void SourcesToolService::getAllStatements() {
    TimeLogger timer("GET /statements/all");

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
        ApprovalState state = ApprovalState::UNAPPROVED;
        if (request().get("state") != "") {
            state = stateFromString(request().get("state"));
        }

        std::unique_ptr<Value> value;
        if (request().get("value") != "") {
            value = std::unique_ptr<Value>(
                    new Value(parser::parseValue(request().get("value"))));
        }

        RETRY({
                  std::vector<Statement> statements =
                          backend.getAllStatements(cache(), offset, limit,
                                                   state,
                                                   request().get("dataset"),
                                                   request().get("property"),
                                                   value.get());
                  if (statements.size() > 0) {
                      serializeStatements(statements);
                  } else {
                      RESPONSE(NOT_FOUND) << "No statements found";
                  }
              }, 3, cppdb::cppdb_error);
    } catch(cppdb::cppdb_error const &e) {
        RESPONSE(SERVER_ERROR) << "internal server error";
    } catch(InvalidApprovalState const &e) {
        RESPONSE(BAD_REQUEST) << "Invalid or missing state parameter";
    }
}


void SourcesToolService::getStatus() {
    wikidata::primarysources::TimeLogger timer("GET /status");

    addCORSHeaders();
    addVersionHeaders();

    Json::Value result;

    std::string dataset = request().get("dataset");
    model::Status status = backend.getStatus(cache(), dataset);

    // show dataset-specific statements count, defaulting to all datasets
    result["dataset"] = (dataset != "") ? dataset : "all";

    result["statements"]["total"] = status.statements().statements();
    result["statements"]["approved"] = status.statements().approved();
    result["statements"]["unapproved"] = status.statements().unapproved();
    result["statements"]["blacklisted"] = status.statements().blacklisted();
    result["statements"]["duplicate"] = status.statements().duplicate();
    result["statements"]["wrong"] = status.statements().wrong();

    // users information
    result["users"] = status.total_users();

    Json::Value topusers(Json::arrayValue);
    for (const auto& entry : status.top_users()) {
        Json::Value v;
        v["name"] = entry.name();
        v["activities"] = entry.activities();
        topusers.append(v);
    }
    result["topusers"] = topusers;

    // system information
    result["system"]["startup"] = status.system().startup();
    result["system"]["version"] = status.system().version();
    result["system"]["cache_hits"] = status.system().cache_hits();
    result["system"]["cache_misses"] = status.system().cache_misses();
    result["system"]["redis_hits"] = status.system().redis_hits();
    result["system"]["redis_misses"] = status.system().redis_misses();
    result["system"]["shared_mem"] = status.system().shared_memory();
    result["system"]["private_mem"] = status.system().private_memory();
    result["system"]["rss"] = status.system().resident_set_size();

    // request statistics
    result["requests"]["getentity"] = status.requests().get_entity();
    result["requests"]["getrandom"] = status.requests().get_random();
    result["requests"]["getstatement"] = status.requests().get_statement();
    result["requests"]["updatestatement"] = status.requests().update_statement();
    result["requests"]["getstatus"] = status.requests().get_status();

    response().content_type("application/json");
    response().out() << result;

    backend.StatusService().AddGetStatusRequest();
}

void SourcesToolService::serializeStatements(const std::vector<Statement> &statements) {
    if(request().http_accept() == "text/vnd.wikidata+tsv"
       || request().http_accept() == "text/tsv") {
        response().content_type("text/vnd.wikidata+tsv");

        serializer::writeTSV(statements.cbegin(), statements.cend(), &response().out());
    } else if(request().http_accept() == "application/vnd.wikidata+json") {
        response().content_type("application/vnd.wikidata+json");

        serializer::writeWikidataJSON(statements.cbegin(), statements.cend(), &response().out());
    } else {
        response().content_type("application/vnd.wikidata.envelope+json");

        serializer::writeEnvelopeJSON(statements.cbegin(), statements.cend(), &response().out());
    }
}

void SourcesToolService::importStatements() {
    addVersionHeaders();

    if (request().request_method() == "POST") {
        // check if token matches
        if (request().get("token") != settings()["token"].str()) {
            RESPONSE(UNAUTHORIZED) << "Invalid authorization token";
            return;
        }

        std::string dataset = request().get("dataset");
        if (dataset == "") {
            RESPONSE(BAD_REQUEST) << "Missing argument: dataset";
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
        RESPONSE(METHOD_NOT_ALLOWED) << request().request_method();
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
            RESPONSE(UNAUTHORIZED) <<  "Invalid authorization token '" << request().get("token") << "'";
            return;
        }

        TimeLogger timer("POST /delete");

        // check if statement exists and update it with new state
        try {
            backend.deleteStatements(cache(), stateFromString(request().get("state")));
        } catch(PersistenceException const &e) {
            RESPONSE(NOT_FOUND) << "Could not delete statements";
        } catch(InvalidApprovalState const &e) {
            RESPONSE(BAD_REQUEST) << "Invalid or missing state parameter";
        }
    } else {
        RESPONSE(METHOD_NOT_ALLOWED) << request().request_method();
        response().set_header("Allow", "POST");
    }
}

void SourcesToolService::addCORSHeaders() {
    response().set_header("Access-Control-Allow-Origin", "*");
}

void SourcesToolService::addVersionHeaders() {
    response().set_header("X-Powered-By",
                          std::string("Wikidata Sources Tool/") + backend.StatusService().Version());
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

    backend.StatusService().AddGetStatusRequest();
}


}  // namespace primarysources
}  // namespace wikidata
