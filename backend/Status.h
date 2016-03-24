// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#ifndef HAVE_STATUS_H
#define HAVE_STATUS_H

#include <stdint.h>
#include <vector>
#include <string>

#include <cppcms/serialization.h>

// Simple container class representing the current status of the backend.
class Status : public cppcms::serializable {
 public:

    Status() { }

    // Get number of current statements.
    int64_t getStatements() const {
        return statements;
    }

    void setStatements(int64_t statements) {
        Status::statements = statements;
    }

    // Get number of approved statements.
    int64_t getApproved() const {
        return approved;
    }

    void setApproved(int64_t approved) {
        Status::approved = approved;
    }

    // Get number of unapproved statements.
    int64_t getUnapproved() const {
        return unapproved;
    }

    void setUnapproved(int64_t unapproved) {
        Status::unapproved = unapproved;
    }

    // Get number of wrong statements.
    int64_t getWrong() const {
        return wrong;
    }

    void setWrong(int64_t wrong) {
        Status::wrong = wrong;
    }

    // Get number of duplicate statements.
    int64_t getDuplicate() const {
        return duplicate;
    }

    void setDuplicate(int64_t duplicate) {
        Status::duplicate = duplicate;
    }

    // Get number of blacklisted statements.
    int64_t getBlacklisted() const {
        return blacklisted;
    }

    void setBlacklisted(int64_t blacklisted) {
        Status::blacklisted = blacklisted;
    }

    // Get a vector of the top users.
    std::vector<std::pair<std::string, int64_t>> const &getTopUsers() const {
        return topUsers;
    }

    void setTopUsers(std::vector<std::pair<std::string, int64_t>> const &topUsers) {
        Status::topUsers = topUsers;
    }

    void serialize(cppcms::archive &a) override;

 private:
    // current numbers of statements (total, and different states) in the
    // database
    int64_t statements, approved, unapproved, duplicate, blacklisted, wrong;

    // top users with count of statements processed
    std::vector<std::pair<std::string,int64_t>> topUsers;
};


#endif //HAVE_STATUS_H
