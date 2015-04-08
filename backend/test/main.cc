//
// Created by schaffert on 4/8/15.
//
#include "gtest.h"

// run all tests in the current binary
int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
