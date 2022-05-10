// Copyright 2021 The Pigweed Authors
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
#pragma once

#include "gmock/gmock.h"
#include "pw_analog/microvolt_input.h"

namespace pw::analog {

class GmockMicrovoltInput : public MicrovoltInput {
 public:
  MOCK_METHOD(pw::Result<int32_t>,
              TryReadUntil,
              (pw::chrono::SystemClock::time_point deadline),
              (override));

  MOCK_METHOD(Limits, GetLimits, (), (const, override));

  MOCK_METHOD(References, GetReferences, (), (const, override));
};

}  // namespace pw::analog
