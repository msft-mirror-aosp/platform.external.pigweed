// Copyright 2020 The Pigweed Authors
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

// This is a simple example of how to write a fuzzer. The target function is
// crafted to demonstrates how the fuzzer can analyze conditional branches and
// incrementally cover more and more code until a defect is found.
//
// See build_and_run_toy_fuzzer.sh for examples of how you can build and run
// this example.

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "pw_string/util.h"

namespace {

// The code to fuzz. This would normally be in separate library.
void toy_example(const char* word1, const char* word2) {
  bool greeted = false;
  if (word1[0] == 'h') {
    if (word1[1] == 'e') {
      if (word1[2] == 'l') {
        if (word1[3] == 'l') {
          if (word1[4] == 'o') {
            greeted = true;
          }
        }
      }
    }
  }
  if (word2[0] == 'w') {
    if (word2[1] == 'o') {
      if (word2[2] == 'r') {
        if (word2[3] == 'l') {
          if (word2[4] == 'd') {
            if (greeted) {
              // Our "defect", simulating a crash.
              __builtin_trap();
            }
          }
        }
      }
    }
  }
}

}  // namespace

// The fuzz target function
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  // We want to split our input into two strings.
  const char* word1 = reinterpret_cast<const char*>(data);

  // If that's not feasible, toss this input. The fuzzer will quickly learn that
  // inputs without null-terminators are uninteresting.
  size_t offset = pw::string::Length(word1, size) + 1;
  if (offset >= size) {
    return 0;
  }

  // Actually, inputs without TWO null terminators are uninteresting.
  const char* word2 = reinterpret_cast<const char*>(&data[offset]);
  size -= offset;
  if (pw::string::Length(word2, size) == size) {
    return 0;
  }

  // Call the code we're targeting!
  toy_example(word1, word2);

  // By convention, the fuzzer always returns zero.
  return 0;
}
