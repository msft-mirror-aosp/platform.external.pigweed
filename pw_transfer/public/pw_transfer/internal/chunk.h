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

#include <optional>

#include "pw_bytes/span.h"
#include "pw_result/result.h"

namespace pw::transfer::internal {

struct Chunk {
  // The initial chunk always has an offset of 0 and no data or status.
  //
  // Pending bytes is required in all read chunks, so that is checked elsewhere.
  constexpr bool IsInitialChunk() const {
    return offset == 0 && data.empty() && !status.has_value();
  }

  // The final chunk from the transmitter sets remaining_bytes to 0 in both Read
  // and Write transfers.
  constexpr bool IsFinalTransmitChunk() const { return remaining_bytes == 0u; }

  uint32_t transfer_id;
  std::optional<uint32_t> pending_bytes;
  std::optional<uint32_t> max_chunk_size_bytes;
  std::optional<uint32_t> min_delay_microseconds;
  uint32_t offset;
  ConstByteSpan data;
  std::optional<uint64_t> remaining_bytes;
  std::optional<Status> status;
};

Status DecodeChunk(ConstByteSpan message, Chunk& chunk);
Result<ConstByteSpan> EncodeChunk(const Chunk& chunk, ByteSpan buffer);

}  // namespace pw::transfer::internal
