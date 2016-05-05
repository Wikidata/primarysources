// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_STATUS_H
#define PRIMARYSOURCES_STATUS_H

#include <model/status.pb.h>

namespace wikidata {
namespace primarysources {
namespace model {

inline UserStatus NewUserStatus(const std::string& user, int64_t activities) {
    UserStatus s;
    s.set_name(user);
    s.set_activities(activities);
    return s;
}

bool operator==(const RequestStatus &lhs, const RequestStatus &rhs);
inline bool operator!=(const RequestStatus &lhs, const RequestStatus &rhs) {
    return !(lhs == rhs);
}

bool operator==(const DatasetStatus &lhs, const DatasetStatus &rhs);
inline bool operator!=(const DatasetStatus &lhs, const DatasetStatus &rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const RequestStatus& s) {
    return os << s.DebugString();
}

bool operator==(const StatementStatus &lhs, const StatementStatus &rhs);
inline bool operator!=(const StatementStatus &lhs, const StatementStatus &rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const StatementStatus& s) {
    return os << s.DebugString();
}

bool operator==(const UserStatus &lhs, const UserStatus &rhs);
inline bool operator!=(const UserStatus &lhs, const UserStatus &rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const UserStatus& s) {
    return os << s.DebugString();
}

bool operator==(const SystemStatus &lhs, const SystemStatus &rhs);
inline bool operator!=(const SystemStatus &lhs, const SystemStatus &rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const SystemStatus& s) {
    return os << s.DebugString();
}

bool operator==(const Status &lhs, const Status &rhs);
inline bool operator!=(const Status &lhs, const Status &rhs) {
    return !(lhs == rhs);
}

inline std::ostream& operator<<(std::ostream& os, const Status& s) {
    return os << s.DebugString();
}


}  // namespace model
}  // namespace primarysources
}  // namespace wikidata


#endif //PRIMARYSOURCES_STATUS_H
