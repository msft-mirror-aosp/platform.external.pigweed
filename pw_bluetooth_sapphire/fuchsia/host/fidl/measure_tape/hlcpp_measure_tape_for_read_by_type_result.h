// Copyright 2024 The Pigweed Authors
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

// Code generated by "measure-tape"; DO NOT EDIT.
//
// See tools/fidl/measure-tape/README.md

// clang-format off
#pragma once

#include <fuchsia/bluetooth/gatt2/cpp/fidl.h>


namespace measure_tape {
namespace fuchsia {
namespace bluetooth {
namespace gatt2 {

struct Size {
  explicit Size(int64_t num_bytes, int64_t num_handles)
    : num_bytes(num_bytes), num_handles(num_handles) {}

  const int64_t num_bytes;
  const int64_t num_handles;
};


// Helper function to measure ::fuchsia::bluetooth::gatt2::ReadByTypeResult.
//
// In most cases, the size returned is a precise size. Otherwise, the size
// returned is a safe upper-bound.
Size Measure(const ::fuchsia::bluetooth::gatt2::ReadByTypeResult& value);



}  // gatt2
}  // bluetooth
}  // fuchsia
}  // measure_tape

