// Copyright 2020 The Pigweed Authors
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

#include "pw_rpc/internal/raw_method_union.h"
#include "pw_rpc/internal/service_method_traits.h"

namespace pw::rpc::internal {

template <auto impl_method, uint32_t method_id>
using RawServiceMethodTraits =
    ServiceMethodTraits<&MethodBaseService<impl_method>::RawMethodFor,
                        impl_method,
                        method_id>;

}  // namespace pw::rpc::internal
