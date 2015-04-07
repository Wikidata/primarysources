#include "Status.h"

void Status::serialize(cppcms::archive &a) {
    a & statements & approved & unapproved & blacklisted & duplicate & wrong & topUsers;
}
