//
// Created by wastl on 25.04.15.
//
#include "Dashboard.h"
#include "Statement.h"

#include <iostream>
#include <sstream>

inline std::string sqlPeriod(int32_t period) {
    std::stringstream s;
    s << period << " days";
    return s.str();
}

namespace Dashboard {


    void ActivityEntry::serialize(cppcms::archive &a) {
        a & date & approved & rejected;
    }


    void ActivityLog::serialize(cppcms::archive &a) {
        a & users & activities;
    }

    void ActivityLog::addEntry(ActivityEntry &&entry) {
        activities.emplace_back(std::move(entry));
    }


    Dashboard::Dashboard(cppdb::session &sql) : sql(sql) {

    }

    ActivityLog Dashboard::getActivityLog(int32_t period, int32_t numperiods, int32_t numusers) {
        sql.begin();

        std::cout << "SQL PERIOD: " << sqlPeriod(-period * numperiods) << std::endl;

        // first, return the top users in the given time period
        cppdb::result rusers = (
                sql << "SELECT user, count(id) AS activities FROM userlog "
                        "WHERE changed > date('now', ?) "
                        "GROUP BY user ORDER BY activities DESC LIMIT ?"
                << sqlPeriod(-period * numperiods) << numusers
        );


        std::set<std::string> users;
        while (rusers.next()) {
            users.emplace(rusers.get<std::string>(0));
        }

        // create ActivityLog
        ActivityLog result(users);

        // for each period, select the user activities
        for (int i = 0; i < numperiods; i++) {
            ActivityEntry entry;

            // approved
            cppdb::result rapproved = (
                    sql << "SELECT date('now', ?) as date, user, count(id) AS activities "
                            "FROM userlog WHERE state = 1 "
                            "AND changed > date('now', ?) AND changed <= datetime('now', ?) "
                            "GROUP BY user ORDER BY activities DESC LIMIT ?"
                    << sqlPeriod(-i * period) << sqlPeriod((-i - 1) * period) << sqlPeriod(-i * period) << numusers
            );

            while (rapproved.next()) {
                std::string user = rapproved.get<std::string>(1);

                if (users.find(user) != users.end()) {
                    entry.date = rapproved.get<std::string>(0);
                    entry.approved[user] = rapproved.get<int32_t>(2);
                }
            }

            // rejected
            cppdb::result rrejected = (
                    sql << "SELECT date('now', ?) as date, user, count(id) AS activities "
                            "FROM userlog WHERE state = 3 "
                            "AND changed > date('now', ?) AND changed <= datetime('now', ?) "
                            "GROUP BY user ORDER BY activities DESC LIMIT ?"
                    << sqlPeriod(-i * period) << sqlPeriod((-i - 1) * period) << sqlPeriod(-i * period) << numusers
            );

            while (rrejected.next()) {
                std::string user = rrejected.get<std::string>(1);

                if (users.find(user) != users.end()) {
                    entry.date = rrejected.get<std::string>(0);
                    entry.rejected[user] = rrejected.get<int32_t>(2);
                }
            }

            result.addEntry(std::move(entry));
        }

        sql.commit();

        std::cout << "ACTIVITYLOG: users " << result.getUsers().size() << std::endl;

        return result;
    }
}