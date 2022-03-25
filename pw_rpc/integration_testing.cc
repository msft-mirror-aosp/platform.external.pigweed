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

#include "pw_rpc/integration_testing.h"

#include <limits>

#include "gtest/gtest.h"
#include "pw_log/log.h"
#include "pw_rpc/integration_test_socket_client.h"
#include "pw_unit_test/logging_event_handler.h"

namespace pw::rpc::integration_test {
namespace {

SocketClientContext<512> context;
unit_test::LoggingEventHandler log_test_events;

}  // namespace

Client& client() { return context.client(); }

Status InitializeClient(int argc, char* argv[], const char* usage_args) {
  unit_test::RegisterEventHandler(&log_test_events);

  if (argc < 2) {
    PW_LOG_INFO("Usage: %s %s", argv[0], usage_args);
    return Status::InvalidArgument();
  }

  const int port = std::atoi(argv[1]);

  if (port <= 0 || port > std::numeric_limits<uint16_t>::max()) {
    PW_LOG_CRITICAL("Port numbers must be between 1 and 65535; %d is invalid",
                    port);
    return Status::InvalidArgument();
  }

  PW_LOG_INFO("Connecting to pw_rpc client at localhost:%d", port);
  return context.Start(port);
}

}  // namespace pw::rpc::integration_test
