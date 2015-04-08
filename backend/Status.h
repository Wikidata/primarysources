// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#ifndef HAVE_STATUS_H
#define HAVE_STATUS_H

#include <stdint.h>
#include <vector>
#include <string>

#include <cppcms/serialization.h>

class Status : public cppcms::serializable {
private:
    // current numbers of statements (total, and different states) in the
    // database
    int64_t statements, approved, unapproved, duplicate, blacklisted, wrong;

    // top users with count of statements processed
    std::vector<std::pair<std::string,int64_t>> topUsers;

public:

    Status() { }


    int64_t getStatements() const {
        return statements;
    }

    void setStatements(int64_t statements) {
        Status::statements = statements;
    }

    int64_t getApproved() const {
        return approved;
    }

    void setApproved(int64_t approved) {
        Status::approved = approved;
    }

    int64_t getUnapproved() const {
        return unapproved;
    }

    void setUnapproved(int64_t unapproved) {
        Status::unapproved = unapproved;
    }

    int64_t getWrong() const {
        return wrong;
    }

    void setWrong(int64_t wrong) {
        Status::wrong = wrong;
    }

    int64_t getDuplicate() const {
        return duplicate;
    }

    void setDuplicate(int64_t duplicate) {
        Status::duplicate = duplicate;
    }

    int64_t getBlacklisted() const {
        return blacklisted;
    }

    void setBlacklisted(int64_t blacklisted) {
        Status::blacklisted = blacklisted;
    }

    std::vector<std::pair<std::string, int64_t>> const &getTopUsers() const {
        return topUsers;
    }

    void setTopUsers(std::vector<std::pair<std::string, int64_t>> const &topUsers) {
        Status::topUsers = topUsers;
    }

    void serialize(cppcms::archive &a) override;
};


#endif //HAVE_STATUS_H
