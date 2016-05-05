// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Status.h"

namespace wikidata {
namespace primarysources {
namespace model {

bool operator==(const RequestStatus &lhs, const RequestStatus &rhs) {
    return lhs.get_entity() == rhs.get_entity() &&
           lhs.get_random() == rhs.get_random() &&
           lhs.get_status() == rhs.get_status() &&
           lhs.get_statement() == rhs.get_statement() &&
           lhs.update_statement() == rhs.update_statement();
}

bool operator==(const StatementStatus &lhs, const StatementStatus &rhs) {
    return lhs.statements() == rhs.statements() &&
           lhs.approved() == rhs.approved() &&
           lhs.unapproved() == rhs.unapproved() &&
           lhs.wrong() == rhs.wrong() &&
           lhs.duplicate() == rhs.duplicate() &&
           lhs.blacklisted() == rhs.blacklisted();
}

bool operator==(const UserStatus &lhs, const UserStatus &rhs) {
    return lhs.name() == rhs.name() && lhs.activities() == rhs.activities();
}

bool operator==(const SystemStatus &lhs, const SystemStatus &rhs) {
    return lhs.cache_hits() == rhs.cache_hits() &&
           lhs.cache_misses() == rhs.cache_misses() &&
           lhs.resident_set_size() == rhs.resident_set_size() &&
           lhs.shared_memory() == rhs.shared_memory() &&
           lhs.private_memory() == rhs.private_memory();
}


bool operator==(const Status &lhs, const Status &rhs) {
    std::set<std::string> all_keys;
    for(auto it = lhs.datasets().begin(); it != lhs.datasets().end(); ++it)
        all_keys.insert(it->first);
    for(auto it = rhs.datasets().begin(); it != rhs.datasets().end(); ++it)
        all_keys.insert(it->first);

    auto lst = lhs.datasets();
    auto rst = rhs.datasets();

    for(auto key = all_keys.begin(); key != all_keys.end(); ++key) {
        auto l = lst.find(*key);
        auto r = rst.find(*key);

        if(l == lst.end() || r == rst.end())
            return false;
        else if(l->second != r->second)
            return false;
    }

    if (lhs.system() != rhs.system()) {
        return false;
    }

    if (lhs.requests() != rhs.requests()) {
        return false;
    }

    if (lhs.total_users() != rhs.total_users()) {
        return false;
    }

    if (lhs.top_users_size() != rhs.top_users_size()) {
        return false;
    }
    for (int i=0; i<lhs.top_users_size(); i++) {
        if (lhs.top_users(i) != rhs.top_users(i)) {
            return false;
        }
    }

    return true;
}

}  // namespace model
}  // namespace primarysources
}  // namespace wikidata