// Copyright 2016 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#ifndef PRIMARYSOURCES_SNOWFLAKE_H
#define PRIMARYSOURCES_SNOWFLAKE_H

namespace wikidata {
namespace primarysources {

// A generator for Twitter Snowflake IDs (cf https://github.com/twitter/snowflake).
int64_t Snowflake(int64_t datacenterId = 0);

}  // namespace primarysources
}  // namespace wikidata


#endif //PRIMARYSOURCES_SNOWFLAKE_H
