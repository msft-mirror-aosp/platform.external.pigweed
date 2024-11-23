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

#include "pw_bluetooth_proxy/internal/acl_data_channel.h"
#include "pw_bluetooth_proxy/internal/h4_storage.h"
#include "pw_bluetooth_proxy/internal/l2cap_read_channel.h"
#include "pw_bluetooth_proxy/internal/l2cap_write_channel.h"

namespace pw::bluetooth::proxy {

// `L2capChannelManager` mediates between `ProxyHost` and the L2CAP-based
// channels held by clients of `ProxyHost`, such as L2CAP connection-oriented
// channels, GATT Notify channels, and RFCOMM channels.
//
// When an L2CAP-based channel is constructed, it registers itself in one or
// both of the lists managed by `L2capChannelManager`: `read_channels_`, for
// channels to which Rx L2CAP packets are to be routed, and/or
// `write_channels_`, for channels exposing packet Tx capabilities to clients.
//
// ACL packet transmission is subject to data control flow, managed by
// `AclDataChannel`. `L2capChannelManager` handles queueing Tx packets when
// credits are unavailable and sending Tx packets as credits become available,
// dequeueing packets in FIFO order per channel and in round robin fashion
// around channels.
class L2capChannelManager {
 public:
  L2capChannelManager(AclDataChannel& acl_data_channel);

  // Reset state.
  void Reset();

  // Start proxying L2CAP packets addressed to `channel` arriving from
  // the controller.
  void RegisterReadChannel(L2capReadChannel& channel);

  // Stop proxying L2CAP packets addressed to `channel`.
  //
  // Returns false if `channel` is not found.
  bool ReleaseReadChannel(L2capReadChannel& channel);

  // Allow `channel` to send & queue Tx L2CAP packets.
  void RegisterWriteChannel(L2capWriteChannel& channel);

  // Stop sending L2CAP packets queued in `channel` and clear its queue.
  //
  // Returns false if `channel` is not found.
  bool ReleaseWriteChannel(L2capWriteChannel& channel);

  // Get an `H4PacketWithH4` backed by a buffer in `H4Storage` able to hold
  // `size` bytes of data.
  //
  // Returns PW_STATUS_UNAVAILABLE if all buffers are currently occupied.
  // Returns PW_STATUS_INVALID_ARGUMENT if `size` is too large for a buffer.
  pw::Result<H4PacketWithH4> GetTxH4Packet(uint16_t size);

  // Send L2CAP packets queued in registered write channels as long as ACL
  // send credits are available.
  void DrainWriteChannelQueues() PW_LOCKS_EXCLUDED(write_channels_mutex_);

  // Returns the size of an H4 buffer reserved for Tx packets.
  uint16_t GetH4BuffSize() const;

  // Returns pointer to L2CAP channel with given `connection_handle` &
  // `remote_cid` if contained in `write_channels_`. Returns nullptr if not
  // found.
  L2capWriteChannel* FindWriteChannel(uint16_t connection_handle,
                                      uint16_t remote_cid);

  // Returns pointer to L2CAP channel with given `connection_handle` &
  // `local_cid` if contained in `read_channels_`. Returns nullptr if not
  // found.
  L2capReadChannel* FindReadChannel(uint16_t connection_handle,
                                    uint16_t local_cid);

 private:
  // Circularly advance `it`, wrapping around to front if `it` reaches the end.
  void Advance(IntrusiveForwardList<L2capWriteChannel>::iterator& it)
      PW_EXCLUSIVE_LOCKS_REQUIRED(write_channels_mutex_);

  // Reference to the ACL data channel owned by the proxy.
  AclDataChannel& acl_data_channel_;

  // Owns H4 packet buffers.
  H4Storage h4_storage_;

  // List of active L2CAP channels to which Rx packets are routed.
  IntrusiveForwardList<L2capReadChannel> read_channels_;

  // Enforce mutual exclusion of all operations on write channels.
  sync::Mutex write_channels_mutex_;

  // List of active L2CAP channels with packet Tx capabilities.
  IntrusiveForwardList<L2capWriteChannel> write_channels_
      PW_GUARDED_BY(write_channels_mutex_);

  // Iterator to "least recently drained" write channel.
  IntrusiveForwardList<L2capWriteChannel>::iterator lrd_write_channel_
      PW_GUARDED_BY(write_channels_mutex_);
};

}  // namespace pw::bluetooth::proxy
