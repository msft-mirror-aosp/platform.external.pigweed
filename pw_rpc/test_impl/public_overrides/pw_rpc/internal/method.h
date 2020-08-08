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
#pragma once

#include <cstdint>
#include <cstring>
#include <span>

#include "pw_rpc/internal/base_method.h"
#include "pw_rpc/internal/packet.h"
#include "pw_rpc/server_context.h"
#include "pw_status/status_with_size.h"

namespace pw::rpc::internal {

// This is a fake RPC method implementation for testing only. It stores the
// channel ID, request, and payload buffer, and optionally provides a response.
class Method : public BaseMethod {
 public:
  constexpr Method(uint32_t id) : BaseMethod(id), last_channel_id_(0) {}

  void Invoke(ServerCall& call, const Packet& request) const {
    last_channel_id_ = call.channel().id();
    last_request_ = request;
  }

  uint32_t last_channel_id() const { return last_channel_id_; }
  const Packet& last_request() const { return last_request_; }

  void set_response(std::span<const std::byte> payload) { response_ = payload; }
  void set_status(Status status) { response_status_ = status; }

 private:
  // Make these mutable so they can be set in the Invoke method, which is const.
  // The Method class is used exclusively in tests. Having these members mutable
  // allows tests to verify that the Method is invoked correctly.
  mutable uint32_t last_channel_id_;
  mutable Packet last_request_;

  std::span<const std::byte> response_;
  Status response_status_;
};

}  // namespace pw::rpc::internal
