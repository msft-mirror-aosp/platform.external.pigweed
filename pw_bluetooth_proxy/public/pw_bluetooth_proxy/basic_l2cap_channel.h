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

#include "pw_bluetooth_proxy/internal/l2cap_read_channel.h"

namespace pw::bluetooth::proxy {

// TODO: https://pwbug.dev/360929142 - Also support L2capWriteChannel.
class BasicL2capChannel : public L2capReadChannel {
 public:
  explicit BasicL2capChannel(
      L2capChannelManager& l2cap_channel_manager,
      uint16_t connection_handle,
      uint16_t local_cid,
      pw::Function<void(pw::span<uint8_t> payload)>&& receive_fn);
  BasicL2capChannel(BasicL2capChannel&&) = default;
  BasicL2capChannel& operator=(BasicL2capChannel&& other) = default;

 protected:
  bool OnPduReceived(pw::span<uint8_t> bframe) override;
};

}  // namespace pw::bluetooth::proxy
