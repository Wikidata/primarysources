//
// Created by wastl on 28.03.16.
//

#include <glog/logging.h>
#include "HttpStatus.h"

namespace wikidata {
namespace primarysources {

std::string codeToString(StatusCode c) {
    switch (c) {
        case OK:
            return "OK";
        case BAD_REQUEST:
            return "Bad Request";
        case UNAUTHORIZED:
            return "Unauthorized";
        case FORBIDDEN:
            return "Forbidden";
        case NOT_FOUND:
            return "Not Found";
        case METHOD_NOT_ALLOWED:
            return "Method Not Allowed";
        case SERVER_ERROR:
            return "Server Error";
        default:
            return "Error";
    }
}

HttpStatus &HttpStatus::operator<<(const std::string &s) {
    messages_ << s;
    return *this;
}

HttpStatus &HttpStatus::operator<<(int64_t v) {
    messages_ << v;
    return *this;
}

HttpStatus &HttpStatus::operator<<(double v) {
    messages_ << v;
    return *this;
}

HttpStatus::~HttpStatus() {
    LOG_IF(ERROR, code_ >= 400)
        << codeToString(code_) << ": " << messages_.str();

    response_->status(code_, messages_.str());
}


}  // namespace primarysources
}  // namespace wikidata


