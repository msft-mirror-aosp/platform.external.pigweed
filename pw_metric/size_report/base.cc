// Copyright 2019 The Pigweed Authors
//
// Licensed under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License. You may obtain a copy of
// the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
// License for the specific language governing permissions and limitations under
// the License.

#include "pw_assert/check.h"
#include "pw_bloat/bloat_this_binary.h"
#include "pw_log/log.h"

int volatile* unoptimizable;

int main() {
  pw::bloat::BloatThisBinary();

  // Ensure we are paying the cost for log and assert.
  PW_CHECK_INT_GE(*unoptimizable, 0, "Ensure this CHECK logic stays");
  PW_LOG_INFO("We care about optimizing: %d", *unoptimizable);
  // This matches the log preventing optimizing the "m" metric in one_metric.cc.
  PW_LOG_INFO("some_metric: %d", *unoptimizable);

  return *unoptimizable;
}
