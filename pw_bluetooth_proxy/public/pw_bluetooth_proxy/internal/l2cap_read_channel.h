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

#pragma once

#include "pw_containers/intrusive_forward_list.h"
#include "pw_function/function.h"
#include "pw_span/span.h"

namespace pw::bluetooth::proxy {

class L2capChannelManager;

// Base class for peer-to-peer L2CAP-based channels supporting reading.
//
// Read channels invoke a client-supplied read callback for packets sent by
// the peer to the channel.
class L2capReadChannel : public IntrusiveForwardList<L2capReadChannel>::Item {
 public:
  L2capReadChannel(const L2capReadChannel& other) = delete;
  L2capReadChannel& operator=(const L2capReadChannel& other) = delete;
  L2capReadChannel(L2capReadChannel&& other);
  // Move assignment operator allows channels to be erased from pw::Vector.
  L2capReadChannel& operator=(L2capReadChannel&& other);

  virtual ~L2capReadChannel();

  // Handle an Rx L2CAP PDU.
  //
  // Implementations should call `CallReceiveFn` after recombining/processing
  // the SDU (e.g. after updating channel state and screening out certain SDUs).
  //
  // Return true if the PDU was consumed by the channel. Otherwise, return false
  // and the PDU will be forwarded by `ProxyHost` on to the Bluetooth host.
  [[nodiscard]] virtual bool OnPduReceived(pw::span<uint8_t> l2cap_pdu) = 0;

  // Handle fragmented Rx L2CAP PDU.
  // TODO: https://pwbug.dev/365179076 - Support recombination & delete this.
  virtual void OnFragmentedPduReceived() = 0;

  // Get the source L2CAP channel ID.
  uint16_t local_cid() const { return local_cid_; }

  // Get the ACL connection handle.
  uint16_t connection_handle() const { return connection_handle_; }

 protected:
  explicit L2capReadChannel(
      L2capChannelManager& l2cap_channel_manager,
      pw::Function<void(pw::span<uint8_t> payload)>&& receive_fn,
      uint16_t connection_handle,
      uint16_t local_cid);

  // Often the useful `payload` for clients is some subspan of the Rx SDU.
  void CallReceiveFn(pw::span<uint8_t> payload) {
    if (receive_fn_) {
      receive_fn_(payload);
    }
  }

 private:
  // ACL connection handle of this channel.
  uint16_t connection_handle_;
  // L2CAP channel ID of this channel.
  uint16_t local_cid_;
  // Client-provided read callback.
  pw::Function<void(pw::span<uint8_t> payload)> receive_fn_;

  L2capChannelManager& l2cap_channel_manager_;
};

}  // namespace pw::bluetooth::proxy
