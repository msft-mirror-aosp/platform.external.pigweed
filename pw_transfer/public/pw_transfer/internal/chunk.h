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
  // TODO(frolv): This is copied from the proto enum. Ideally, this entire class
  // would be generated by pw_protobuf.
  enum Type {
    kTransferData = 0,
    kTransferStart = 1,
    kParametersRetransmit = 2,
    kParametersContinue = 3,
    kTransferCompletion = 4,
    kTransferCompletionAck = 5,  // Currently unused.
  };

  // The initial chunk always has an offset of 0 and no data or status.
  //
  // TODO(frolv): Going forward, all users of transfer should set a type for
  // all chunks. This initial chunk assumption should be removed.
  constexpr bool IsInitialChunk() const {
    return type == Type::kTransferStart ||
           (offset == 0 && data.empty() && !status.has_value());
  }

  // The final chunk from the transmitter sets remaining_bytes to 0 in both Read
  // and Write transfers.
  constexpr bool IsFinalTransmitChunk() const { return remaining_bytes == 0u; }

  uint32_t transfer_id;
  uint32_t window_end_offset;
  std::optional<uint32_t> pending_bytes;
  std::optional<uint32_t> max_chunk_size_bytes;
  std::optional<uint32_t> min_delay_microseconds;
  uint32_t offset;
  ConstByteSpan data;
  std::optional<uint64_t> remaining_bytes;
  std::optional<Status> status;
  std::optional<Type> type;
};

// Partially decodes a transfer chunk to find its transfer ID field.
Result<uint32_t> ExtractTransferId(ConstByteSpan message);

Status DecodeChunk(ConstByteSpan message, Chunk& chunk);
Result<ConstByteSpan> EncodeChunk(const Chunk& chunk, ByteSpan buffer);

}  // namespace pw::transfer::internal