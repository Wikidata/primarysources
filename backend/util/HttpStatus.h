//
// Created by wastl on 28.03.16.
//

#ifndef PRIMARYSOURCES_HTTPSTATUS_H
#define PRIMARYSOURCES_HTTPSTATUS_H

#include <cppcms/http_response.h>

namespace wikidata {
namespace primarysources {

enum StatusCode {
    OK = 200,
    BAD_REQUEST = 400,
    UNAUTHORIZED = 401,
    FORBIDDEN = 403,
    NOT_FOUND = 404,
    METHOD_NOT_ALLOWED = 405,
    SERVER_ERROR = 500
};

class HttpStatus {
 public:

    HttpStatus(cppcms::http::response* response, StatusCode code)
        : response_(response), code_(code) {};

    ~HttpStatus();

    HttpStatus& operator<<(const std::string& s);
    HttpStatus& operator<<(int64_t v);
    HttpStatus& operator<<(double v);
 private:
    StatusCode code_;
    cppcms::http::response* response_;

    std::stringstream messages_;
};

// Macro for use inside cppcms service.
#define RESPONSE(code) HttpStatus(&response(), code)

}  // namespace primarysources
}  // namespace wikidata

#endif //PRIMARYSOURCES_HTTPSTATUS_H
