// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>
#include "gtest/gtest.h"
#include "util/Retry.h"

namespace wikidata {
namespace primarysources {
namespace {
class Mock {
 public:
    Mock() : calls_(0) {};

    void Fail() {
        calls_++;
        throw std::exception();
    }

    void Succeed() {
        calls_++;
    }

    int Calls() const {
        return calls_;
    }
 private:
    int calls_;
};
}

TEST(RetryTest, Fail) {
    Mock m;
    EXPECT_THROW(RETRY(m.Fail(), 3, std::exception), std::exception);
    EXPECT_EQ(4, m.Calls());
}

}  // namespace primarysources
}  // namespace wikidata

