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

#include "pw_bluetooth_proxy/l2cap_coc.h"

namespace pw::bluetooth::proxy {

class L2capCocInternal final : public L2capCoc {
 public:
  // Should only be created by `ProxyHost` and tests.
  static pw::Result<L2capCoc> Create(
      L2capChannelManager& l2cap_channel_manager,
      uint16_t connection_handle,
      CocConfig rx_config,
      CocConfig tx_config,
      pw::Function<void(pw::span<uint8_t> payload)>&& receive_fn,
      pw::Function<void(Event event)>&& event_fn) {
    return L2capCoc::Create(l2cap_channel_manager,
                            connection_handle,
                            rx_config,
                            tx_config,
                            std::move(receive_fn),
                            std::move(event_fn));
  }

  // Increment L2CAP credits. This should be called by signaling channels in
  // response to L2CAP_FLOW_CONTROL_CREDIT_IND packets.
  void AddCredits(uint16_t credits) { L2capCoc::AddCredits(credits); }
};

}  // namespace pw::bluetooth::proxy
