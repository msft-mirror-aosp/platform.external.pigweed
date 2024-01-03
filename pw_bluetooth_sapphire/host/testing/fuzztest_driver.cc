// Copyright 2023 The Pigweed Authors
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

#include "pw_bluetooth_sapphire/internal/host/common/log.h"

// Prevent "undefined symbol: __zircon_driver_rec__" error.
#ifdef PW_LOG_DECLARE_FAKE_DRIVER
PW_LOG_DECLARE_FAKE_DRIVER();
#endif

// Entry point for libFuzzer that switches logging to printf output with lower
// verbosity.
extern "C" int LLVMFuzzerInitialize(int* argc, char*** argv) {
  bt::UsePrintf(bt::LogSeverity::ERROR);
  return 0;
}
