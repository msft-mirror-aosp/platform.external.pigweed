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

#include <span>

#include "pw_containers/intrusive_list.h"
#include "pw_result/result.h"
#include "pw_rpc/internal/call.h"
#include "pw_rpc/internal/channel.h"
#include "pw_rpc/internal/packet.h"

namespace pw::rpc::internal {

// Manages a list of channels and a list of ongoing calls for either a server or
// client.
//
// For clients, calls start when they send a REQUEST packet to a server. For
// servers, calls start when the REQUEST packet is received. In either case,
// calls add themselves to the Endpoint's list when they're started and
// remove themselves when they complete. Calls do this through their associated
// Server or Client object, which derive from Endpoint.
class Endpoint {
 public:
  ~Endpoint();

  // Finds an RPC Channel with this ID or nullptr if none matches.
  rpc::Channel* GetChannel(uint32_t id) const { return GetInternalChannel(id); }

 protected:
  constexpr Endpoint(std::span<rpc::Channel> channels)
      : channels_(static_cast<internal::Channel*>(channels.data()),
                  channels.size()),
        next_call_id_(0) {}

  // Parses an RPC packet and sets ongoing_call to the matching call, if any.
  // Returns the parsed packet or an error.
  Result<Packet> ProcessPacket(std::span<const std::byte> data,
                               Packet::Destination packet);

  // Finds a call object for an ongoing call associated with this packet, if
  // any. Returns nullptr if no matching call exists.
  Call* FindCall(const Packet& packet) {
    return FindCallById(
        packet.channel_id(), packet.service_id(), packet.method_id());
  }

  // Finds an internal:::Channel with this ID or nullptr if none matches.
  Channel* GetInternalChannel(uint32_t id) const;

  // Creates a channel with the provided ID and ChannelOutput, if a channel slot
  // is available. Returns a pointer to the channel if one is created, nullptr
  // otherwise.
  Channel* AssignChannel(uint32_t id, ChannelOutput& interface);

 private:
  // Give Call access to the register/unregister functions.
  friend class Call;

  // Returns an ID that can be assigned to a new call.
  uint32_t NewCallId() {
    // Call IDs are varint encoded. Limit the varint size to 2 bytes (14 usable
    // bits).
    constexpr uint32_t kMaxCallId = 1 << 14;
    return (++next_call_id_) % kMaxCallId;
  }

  // Adds a call to the internal call registry. If a matching call already
  // exists, it is cancelled locally (on_error called, no packet sent).
  void RegisterCall(Call& call);

  // Registers a call that is known to be unique. The calls list is NOT checked
  // for existing calls.
  void RegisterUniqueCall(Call& call) { calls_.push_front(call); }

  // Removes the provided call from the call registry.
  void UnregisterCall(const Call& call) { calls_.remove(call); }

  Call* FindCallById(uint32_t channel_id,
                     uint32_t service_id,
                     uint32_t method_id);

  std::span<Channel> channels_;
  IntrusiveList<Call> calls_;

  uint32_t next_call_id_;
};

}  // namespace pw::rpc::internal
