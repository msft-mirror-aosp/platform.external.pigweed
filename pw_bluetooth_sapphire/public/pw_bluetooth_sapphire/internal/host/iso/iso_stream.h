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

#include <cstdint>

#include "pw_bluetooth_sapphire/internal/host/common/weak_self.h"
#include "pw_bluetooth_sapphire/internal/host/hci-spec/protocol.h"
#include "pw_bluetooth_sapphire/internal/host/iso/iso_common.h"
#include "pw_bluetooth_sapphire/internal/host/transport/command_channel.h"
#include "pw_bluetooth_sapphire/internal/host/transport/transport.h"

namespace bt::iso {

class IsoStream : public hci::IsoDataChannel::ConnectionInterface {
 public:
  virtual ~IsoStream() = default;

  // Handler for incoming HCI_LE_CIS_Established events. Returns a value
  // indicating whether the vent was handled.
  virtual bool OnCisEstablished(const hci::EventPacket& event) = 0;

  enum SetupDataPathError {
    kSuccess,
    kStreamAlreadyExists,
    kCisNotEstablished,
    kStreamRejectedByController,
    kInvalidArgs,
    kStreamClosed,
  };

  using SetupDataPathCallback = pw::Callback<void(SetupDataPathError)>;
  using IncomingDataHandler =
      pw::Function<bool(const pw::span<const std::byte>&)>;
  virtual void SetupDataPath(
      pw::bluetooth::emboss::DataPathDirection direction,
      const bt::StaticPacket<pw::bluetooth::emboss::CodecIdWriter>& codec_id,
      const std::optional<std::vector<uint8_t>>& codec_configuration,
      uint32_t controller_delay_usecs,
      SetupDataPathCallback&& on_complete_cb,
      IncomingDataHandler&& on_incoming_data_available_cb) = 0;

  virtual hci_spec::ConnectionHandle cis_handle() const = 0;

  // Terminate this stream.
  virtual void Close() = 0;

  static std::unique_ptr<IsoStream> Create(
      uint8_t cig_id,
      uint8_t cis_id,
      hci_spec::ConnectionHandle cis_handle,
      CisEstablishedCallback on_established_cb,
      hci::CommandChannel::WeakPtr cmd,
      pw::Callback<void()> on_closed_cb);

  using WeakPtr = WeakSelf<IsoStream>::WeakPtr;
  virtual WeakPtr GetWeakPtr() = 0;
};

}  // namespace bt::iso
