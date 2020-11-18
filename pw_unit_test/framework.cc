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

#include "pw_unit_test/framework.h"

#include <cstring>

namespace pw {
namespace unit_test {

void RegisterEventHandler(EventHandler* event_handler) {
  internal::Framework::Get().RegisterEventHandler(event_handler);
}

namespace internal {

// Singleton instance of the unit test framework class.
Framework Framework::framework_;

// Linked list of all test cases in the test executable. This is static as it is
// populated using static initialization.
TestInfo* Framework::tests_ = nullptr;

void Framework::RegisterTest(TestInfo* new_test) {
  // If the test list is empty, set new_test as the first test.
  if (tests_ == nullptr) {
    tests_ = new_test;
    return;
  }

  // Append the test case to the end of the test list.
  TestInfo* info = tests_;
  for (; info->next() != nullptr; info = info->next()) {
  }
  info->set_next(new_test);
}

int Framework::RunAllTests() {
  run_tests_summary_.passed_tests = 0;
  run_tests_summary_.failed_tests = 0;

  if (event_handler_ != nullptr) {
    event_handler_->RunAllTestsStart();
  }
  for (const TestInfo* test = tests_; test != nullptr; test = test->next()) {
    if (test->enabled()) {
      test->run();
    } else {
      event_handler_->TestCaseDisabled(test->test_case());
    }
  }
  if (event_handler_ != nullptr) {
    event_handler_->RunAllTestsEnd(run_tests_summary_);
  }
  return exit_status_;
}

void Framework::StartTest(const TestInfo& test) {
  current_test_ = &test;
  current_result_ = TestResult::kSuccess;

  if (event_handler_ != nullptr) {
    event_handler_->TestCaseStart(test.test_case());
  }
}

void Framework::EndCurrentTest() {
  switch (current_result_) {
    case TestResult::kSuccess:
      run_tests_summary_.passed_tests++;
      break;
    case TestResult::kFailure:
      run_tests_summary_.failed_tests++;
      break;
  }

  if (event_handler_ != nullptr) {
    event_handler_->TestCaseEnd(current_test_->test_case(), current_result_);
  }

  current_test_ = nullptr;
}

void Framework::ExpectationResult(const char* expression,
                                  const char* evaluated_expression,
                                  int line,
                                  bool success) {
  if (!success) {
    current_result_ = TestResult::kFailure;
    exit_status_ = 1;
  }

  if (event_handler_ == nullptr) {
    return;
  }

  TestExpectation expectation = {
      .expression = expression,
      .evaluated_expression = evaluated_expression,
      .line_number = line,
      .success = success,
  };

  event_handler_->TestCaseExpect(current_test_->test_case(), expectation);
}

bool TestInfo::enabled() const {
  constexpr size_t kStringSize = sizeof("DISABLED_") - 1;
  return std::strncmp("DISABLED_", test_case().test_name, kStringSize) != 0 &&
         std::strncmp("DISABLED_", test_case().suite_name, kStringSize) != 0;
}

}  // namespace internal
}  // namespace unit_test
}  // namespace pw
