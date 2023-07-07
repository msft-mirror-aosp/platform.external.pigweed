// Copyright 2022 The Pigweed Authors
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

#include "pw_rpc/pwpb/server_reader_writer.h"

#include "pw_rpc/pwpb/internal/method.h"

namespace pw::rpc::internal {

PwpbServerCall::PwpbServerCall(const LockedCallContext& context,
                               MethodType type)
    : ServerCall(context, CallProperties(type, kServerCall, kProtoStruct)),
      serde_(&static_cast<const PwpbMethod&>(context.method()).serde()) {}

}  // namespace pw::rpc::internal
