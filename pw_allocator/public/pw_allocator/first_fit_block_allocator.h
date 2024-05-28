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

#include "pw_allocator/block_allocator_base.h"

namespace pw::allocator {

/// Block allocator that uses a "first-fit" allocation strategy.
///
/// In this strategy, the allocator handles an allocation request by starting at
/// the beginning of the range of blocks and looking for the first one which can
/// satisfy the request.
///
/// This strategy may result in slightly worse fragmentation than the
/// corresponding "last-fit" strategy, since the alignment may result in unused
/// fragments both before and after an allocated block.
template <typename OffsetType = uintptr_t,
          size_t kPoisonInterval = 0,
          size_t kAlign = alignof(OffsetType)>
class FirstFitBlockAllocator
    : public BlockAllocator<OffsetType, kPoisonInterval, kAlign> {
 public:
  using Base = BlockAllocator<OffsetType, kPoisonInterval, kAlign>;
  using BlockType = typename Base::BlockType;

  /// Constexpr constructor. Callers must explicitly call `Init`.
  constexpr FirstFitBlockAllocator() : Base() {}

  /// Non-constexpr constructor that automatically calls `Init`.
  ///
  /// @param[in]  region  Region of memory to use when satisfying allocation
  ///                     requests. The region MUST be large enough to fit an
  ///                     aligned block with overhead. It MUST NOT be larger
  ///                     than what is addressable by `OffsetType`.
  explicit FirstFitBlockAllocator(ByteSpan region) : Base(region) {}

 private:
  /// @copydoc Allocator::Allocate
  BlockType* ChooseBlock(Layout layout) override {
    // Search forwards for the first block that can hold this allocation.
    for (auto* block : Base::blocks()) {
      block->CrashIfInvalid();
      if (BlockType::AllocFirst(block, layout).ok()) {
        return block;
      }
    }
    return nullptr;
  }
};

}  // namespace pw::allocator
