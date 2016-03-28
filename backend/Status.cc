// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "Status.h"

void Status::serialize(cppcms::archive &a) {
    a & statements & approved & unapproved & blacklisted & duplicate & wrong & users & topUsers;
}
