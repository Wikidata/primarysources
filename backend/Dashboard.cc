//
// Created by wastl on 25.04.15.
//
#include <ctime>

#include "Dashboard.h"
#include "model/Statement.h"

#include <iostream>

inline std::string sqlPeriod(int32_t period) {
    return std::to_string(period) + " days";
}

// Format a date relative to 'period' in ISO day format. E.g. when period is -7,
// return the date of one week ago.
inline std::string pastDate(int32_t period) {
    std::time_t rawtime;
    std::time(&rawtime);

    rawtime += period * 24 * 60 * 60;

    std::tm* ptm = std::gmtime(&rawtime);

    char past_date[11];
    std::strftime(past_date, 11, "%Y-%m-%d", ptm);
    return std::string(past_date);
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
            ActivityEntry entry(pastDate(-i * period));

            // approved
            cppdb::result rapproved = (
                    sql << "SELECT user, count(id) AS activities "
                            "FROM userlog WHERE state = 1 "
                            "AND changed > date('now', ?) AND changed <= datetime('now', ?) "
                            "GROUP BY user ORDER BY activities DESC LIMIT ?"
                    << sqlPeriod((-i - 1) * period) << sqlPeriod(-i * period) << numusers
            );

            while (rapproved.next()) {
                std::string user = rapproved.get<std::string>(0);

                if (users.find(user) != users.end()) {
                    entry.approved[user] = rapproved.get<int32_t>(1);
                }
            }

            // rejected
            cppdb::result rrejected = (
                    sql << "SELECT user, count(id) AS activities "
                            "FROM userlog WHERE state = 3 "
                            "AND changed > date('now', ?) AND changed <= datetime('now', ?) "
                            "GROUP BY user ORDER BY activities DESC LIMIT ?"
                    << sqlPeriod((-i - 1) * period) << sqlPeriod(-i * period) << numusers
            );

            while (rrejected.next()) {
                std::string user = rrejected.get<std::string>(0);

                if (users.find(user) != users.end()) {
                    entry.rejected[user] = rrejected.get<int32_t>(1);
                }
            }

            result.addEntry(std::move(entry));
        }

        sql.commit();

        return result;
    }
}