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

#include "pw_bluetooth_proxy/internal/l2cap_write_channel.h"

namespace pw::bluetooth::proxy {

/// L2CAP connection-oriented channel that supports writing to and reading
/// from a remote peer.
///
// TODO: https://pwbug.dev/360934030 - Support queuing + credit-based control
// flow.
class L2capCoc : public L2capWriteChannel {
 public:
  /// Parameters for a direction of packet flow in an `L2capCoc`.
  struct CocConfig {
    /// Channel identifier of the endpoint.
    /// For Rx: Local CID.
    /// For Tx: Remote CID.
    uint16_t cid;
    /// Maximum Transmission Unit.
    /// For Rx: Specified by local device. Indicates the maximum SDU size we are
    ///         capable of accepting.
    /// For Tx: Specified by remote peer. Indicates the maximum SDU size we are
    ///         allowed to send.
    uint16_t mtu;
    /// Maximum PDU payload Size.
    /// For Rx: Specified by local device. Indicates the maximum payload size
    ///         for an L2CAP packet we are capable of accepting.
    /// For Tx: Specified by remote peer. Indicates the maximum payload size for
    ///         for an L2CAP packet we are allowed to send.
    uint16_t mps;
    /// For Rx: Tracks the number of credits we have currently apportioned to
    ///         the remote peer for sending us K-frames in LE Credit Based Flow
    ///         Control mode.
    /// For Tx: Currently available credits for sending K-frames in LE Credit
    ///         Based Flow Control mode. This may be different from the initial
    ///         value if the container has already sent K-frames and/or received
    ///         credits.
    uint16_t credits;
  };

  // TODO: saeedali@ - Add error codes here.
  enum class Event {};

  /// Enter `kStopped` state. This means
  ///   - Pending sends will not complete.
  ///   - Calls to `Write()` will return PW_STATUS_FAILED_PRECONDITION.
  ///   - Incoming packets will be dropped & trigger `kRxWhileStopped` events.
  ///   - Container is responsible for closing L2CAP connection & destructing
  ///     the channel object to free its resources.
  ///
  /// .. pw-status-codes::
  ///  OK:               If channel entered `kStopped` state.
  ///  INVALID_ARGUMENT: If channel was previously `kStopped`.
  /// @endrst
  pw::Status Stop();

  /// Send an L2CAP payload to the remote peer.
  ///
  /// @param[in] payload The L2CAP payload to be sent. Payload will be copied
  ///                    before function completes.
  ///
  /// @returns @rst
  ///
  /// .. pw-status-codes::
  ///  OK:                  If packet was successfully queued for send.
  ///  UNAVAILABLE:         If channel could not acquire the resources to queue
  ///  the
  ///                       send at this time (transient error).
  ///  INVALID_ARGUMENT:    If payload is too large.
  ///  FAILED_PRECONDITION: If channel is `kStopped`.
  /// @endrst
  pw::Status Write(pw::span<const uint8_t> payload);

  // TODO: https://pwbug.dev/360934032 - Implement Read().

 protected:
  static pw::Result<L2capCoc> Create(
      AclDataChannel& acl_data_channel,
      H4Storage& h4_storage,
      uint16_t connection_handle,
      CocConfig rx_config,
      CocConfig tx_config,
      pw::Function<void(Event event)>&& event_fn);

 private:
  enum class CocState {
    kRunning,
    kStopped,
  };

  explicit L2capCoc(AclDataChannel& acl_data_channel,
                    H4Storage& h4_storage,
                    uint16_t connection_handle,
                    CocConfig rx_config,
                    CocConfig tx_config,
                    pw::Function<void(Event event)>&& event_fn);

  CocState state_;
  uint16_t tx_mtu_;
  uint16_t tx_mps_;
  pw::Function<void(Event event)> event_fn_;
};

}  // namespace pw::bluetooth::proxy
