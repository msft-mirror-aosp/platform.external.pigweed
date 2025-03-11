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

#include <type_traits>

#include "pw_result/result.h"
#include "pw_span/span.h"
#include "pw_status/status.h"

namespace pw::bluetooth {

// Create an Emboss view and check that it is Ok().
// Returns Status::DataLoss() if the view is not Ok().
//
// The emboss type is determined by the template's first parameter.
template <typename EmbossT, typename... Params>
constexpr inline Result<EmbossT> MakeEmbossView(Params&&... params) {
  auto view = EmbossT(std::forward<Params>(params)...);
  if (view.Ok()) {
    return view;
  } else {
    return Status::DataLoss();
  }
}

// Create an Emboss View from a pw::span value or reference and check that it is
// Ok(). Returns Status::DataLoss() if the view is not Ok().
//
// The emboss type is determined by the template's first parameter.
// Unlike the Emboss `Make*View` creation methods, this function accepts a
// reference so it can be used with rvalues. This is ok to do with pw::span
// since it doesn't own its underlying data.
template <typename EmbossT,
          typename ContainerT,
          typename = std::enable_if_t<
              std::is_convertible_v<ContainerT, pw::span<const uint8_t>>>>
constexpr inline Result<EmbossT> MakeEmbossView(ContainerT&& buffer) {
  return MakeEmbossView<EmbossT>(buffer.data(), buffer.size());
}

// Create an Emboss Writer and check that and check that the
// backing storage contains at least enough space for MinSizeInBytes().
// Returns Status::InvalidArgument() if the buffer isn't large enough for
// requested writer.
//
// The emboss type is determined by the template's first parameter.
template <typename EmbossT, typename... Params>
constexpr inline Result<EmbossT> MakeEmbossWriter(Params&&... params) {
  auto view = EmbossT(std::forward<Params>(params)...);
  if (view.BackingStorage().SizeInBytes() >=
      static_cast<size_t>(EmbossT::MinSizeInBytes().Read())) {
    return view;
  } else {
    return Status::InvalidArgument();
  }
}

// Create an Emboss Writer from a pw::span value or reference and check that the
// backing storage contains at least enough space for MinSizeInBytes(). Returns
// Status::InvalidArgument() if the buffer isn't large enough for requested
// writer.
//
// The emboss type is determined by the template's first parameter.
// Unlike the Emboss `Make*View` creation methods, this function accepts a
// reference so it can be used with rvalues. This is ok to do with pw::span
// since it doesn't own its underlying data.
template <typename EmbossT,
          typename ContainerT,
          typename = std::enable_if_t<
              std::is_convertible_v<ContainerT, pw::span<uint8_t>>>>
constexpr inline Result<EmbossT> MakeEmbossWriter(ContainerT&& buffer) {
  return MakeEmbossWriter<EmbossT>(buffer.data(), buffer.size());
}

}  // namespace pw::bluetooth
