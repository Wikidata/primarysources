// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef FREEBASE_BACKEND_DASHBOARD_H
#define FREEBASE_BACKEND_DASHBOARD_H

#include <string>
#include <vector>
#include <map>
#include <set>

#include <cppdb/frontend.h>
#include <cppcms/serialization.h>

namespace Dashboard {

    struct ActivityEntry : public cppcms::serializable {
        // date of the entry
        std::string date;

        // count of approved and rejected statements per top users
        std::map<std::string, int32_t> approved, rejected;

        ActivityEntry() {}
        ActivityEntry(std::string date) : date(date) {}

        void serialize(cppcms::archive &a) override;
    };

    class ActivityLog : public cppcms::serializable {
    public:

        ActivityLog() { }

        ActivityLog(std::set<std::string>& users)
                : users(users) { }

        void addEntry(ActivityEntry&& entry);


        std::set<std::string> const &getUsers() const {
            return users;
        }

        std::vector<ActivityEntry> const &getActivities() const {
            return activities;
        }

        void serialize(cppcms::archive &a) override;

    private:
        std::set<std::string> users;
        std::vector<ActivityEntry> activities;
    };


    /**
    * Abstraction class for dashboard database queries. Offers low-level methods for
    * querying and activity data in a given CppDB SQL session.
    */
    class Dashboard {
    public:

        /**
         * Create a new persistence layer using the given database session. The
         * dashboard connection takes care of transaction handling.
         */
        Dashboard(cppdb::session &sql);


        /**
         * Query the database for an activity log using the given period in days,
         * returning numperiod periods, and collecting data for the top numusers users.
         */
        ActivityLog getActivityLog(int32_t period, int32_t numperiods, int32_t numusers);

    private:
        // reference to the wrapped sql session
        cppdb::session& sql;

    };
}
#endif //FREEBASE_BACKEND_DASHBOARD_H
