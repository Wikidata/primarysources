// Copyright 2015 Google Inc. All Rights Reserved.
// Author: Sebastian Schaffert <schaffert@google.com>

#include "Statement.h"

void Value::serialize(cppcms::archive &a) {
    a & str & lang & cppcms::as_pod(time) & loc & precision
      & cppcms::as_pod(quantity) & cppcms::as_pod(type);
}

void PropertyValue::serialize(cppcms::archive &a) {
    a & property & value;
}

void Statement::serialize(cppcms::archive &a) {
    a & id & qid & propertyValue & qualifiers & sources
      & cppcms::as_pod(approved);
}
