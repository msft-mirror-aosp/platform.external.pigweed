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

#include "pw_transfer/internal/chunk_data_buffer.h"

#include <algorithm>
#include <cstring>

#include "pw_assert/assert.h"

namespace pw::transfer::internal {

void ChunkDataBuffer::Write(ConstByteSpan data, bool last_chunk) {
  PW_DASSERT(data.size() <= buffer_.size());

  std::copy(data.begin(), data.end(), buffer_.begin());
  size_ = data.size();

  last_chunk_ = last_chunk;
}

}  // namespace pw::transfer::internal
