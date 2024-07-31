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

#include <memory>

#include "pw_bluetooth/controller.h"
#include "pw_bluetooth_sapphire/internal/host/common/weak_self.h"
#include "pw_bluetooth_sapphire/internal/host/transport/command_channel.h"
#include "pw_bluetooth_sapphire/internal/host/transport/data_buffer_info.h"

namespace bt::hci {

// Represents the Bluetooth Isochronous Data channel and manages the
// Host->Controller Isochronous data flow when it is not offloaded.
// IsoDataChannel uses a pull model, where packets are queued in the connections
// and only read by IsoDataChannel when controller buffer space is available.
class IsoDataChannel {
 public:
  // Registered ISO connections must implement this interface to send and
  // receive packets.
  class ConnectionInterface {
   public:
    virtual ~ConnectionInterface() = default;

    // This method will be called when a packet is received for this connection.
    virtual void ReceiveInboundPacket(
        /* std::unique_ptr<IsoDataPacket> packet */) = 0;
  };

  static std::unique_ptr<IsoDataChannel> Create(
      const DataBufferInfo& buffer_info,
      CommandChannel* command_channel,
      pw::bluetooth::Controller* hci);

  // Register a new connection to receive all traffic destined for |handle|. If
  // a connection already exists with this handle, it will not be registered and
  // the previous connection will continue to receive all traffiic for that
  // handle. Returns a value indicating success.
  virtual bool RegisterConnection(hci_spec::ConnectionHandle handle,
                                  WeakPtr<ConnectionInterface> connection) = 0;

  // Unregister a connection when it is disconnected. Returns a value indicating
  // if the connection was recognized and successfully unregistered.
  virtual bool UnregisterConnection(hci_spec::ConnectionHandle handle) = 0;

  virtual ~IsoDataChannel() = default;
};

}  // namespace bt::hci
