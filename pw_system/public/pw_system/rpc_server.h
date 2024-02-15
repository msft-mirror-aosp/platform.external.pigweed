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

#include <cstdint>

#include "pw_rpc/server.h"
#include "pw_system/config.h"
#include "pw_thread/thread_core.h"

namespace pw::system {

// This is the default channel used by the pw_system RPC server. Some other
// parts of pw_system use this channel ID as the default destination for
// unrequested data streams.
inline constexpr uint32_t kDefaultRpcChannelId = PW_SYSTEM_DEFAULT_CHANNEL_ID;

// This is the channel ID used for logging.
inline constexpr uint32_t kLoggingRpcChannelId = PW_SYSTEM_LOGGING_CHANNEL_ID;
#if PW_SYSTEM_EXTRA_LOGGING_CHANNEL_ID != PW_SYSTEM_LOGGING_CHANNEL_ID
inline constexpr uint32_t kExtraLoggingRpcChannelId =
    PW_SYSTEM_EXTRA_LOGGING_CHANNEL_ID;
#endif

rpc::Server& GetRpcServer();

thread::ThreadCore& GetRpcDispatchThread();

}  // namespace pw::system
