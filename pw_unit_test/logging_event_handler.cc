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

#include "pw_unit_test/logging_event_handler.h"

//#include <cstdarg>
//#include <cstdio>
//#include <string_view>
#include <cstdint>

#include "pw_log/log.h"

namespace pw::unit_test {

void LoggingEventHandler::RunAllTestsStart() {
  PW_LOG_INFO("[==========] Running all tests.");
}

void LoggingEventHandler::RunAllTestsEnd(
    const RunTestsSummary& run_tests_summary) {
  PW_LOG_INFO("[==========] Done running all tests.");
  PW_LOG_INFO("[  PASSED  ] %d test(s).", run_tests_summary.passed_tests);
  if (run_tests_summary.failed_tests) {
    PW_LOG_ERROR("[  FAILED  ] %d test(s).", run_tests_summary.failed_tests);
  }
}

void LoggingEventHandler::TestCaseStart(const TestCase& test_case) {
  PW_LOG_INFO("[ RUN      ] %s.%s", test_case.suite_name, test_case.test_name);
}

void LoggingEventHandler::TestCaseEnd(const TestCase& test_case,
                                      TestResult result) {
  // Use a switch with no default to detect changes in the test result enum.
  switch (result) {
    case TestResult::kSuccess:
      PW_LOG_INFO(
          "[       OK ] %s.%s", test_case.suite_name, test_case.test_name);
      break;
    case TestResult::kFailure:
      PW_LOG_ERROR(
          "[  FAILED  ] %s.%s", test_case.suite_name, test_case.test_name);
      break;
  }
}

void LoggingEventHandler::TestCaseExpect(const TestCase& test_case,
                                         const TestExpectation& expectation) {
  if (!verbose_ && expectation.success) {
    return;
  }

  const char* result = expectation.success ? "Success" : "Failure";
  uint32_t level = expectation.success ? PW_LOG_LEVEL_INFO : PW_LOG_LEVEL_ERROR;
  PW_LOG(level,
         PW_LOG_FLAGS,
         "%s:%d: %s",
         test_case.file_name,
         expectation.line_number,
         result);
  PW_LOG(level, PW_LOG_FLAGS, "      Expected: %s", expectation.expression);
  PW_LOG(level,
         PW_LOG_FLAGS,
         "        Actual: %s",
         expectation.evaluated_expression);
}

void LoggingEventHandler::TestCaseDisabled(const TestCase& test) {
  PW_LOG_DEBUG("Skipping disabled test %s.%s", test.suite_name, test.test_name);
}

}  // namespace pw::unit_test
