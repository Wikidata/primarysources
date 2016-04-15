// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include <gtest/gtest.h>
#include <glog/logging.h>

// run all tests in the current binary
int main(int argc, char **argv) {
    // Initialize Google's logging library.
    google::InitGoogleLogging(argv[0]);
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
