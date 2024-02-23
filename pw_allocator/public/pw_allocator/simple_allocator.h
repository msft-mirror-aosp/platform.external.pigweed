// Copyright 2023 The Pigweed Authors
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

#include <cstddef>

#include "pw_allocator/allocator.h"
#include "pw_allocator/block.h"
#include "pw_result/result.h"
#include "pw_status/status.h"

namespace pw::allocator {

// DOCSTAG: [pw_allocator_examples_simple_allocator]
/// Simple allocator that uses a list of `Block`s.
class SimpleAllocator : public Allocator {
 public:
  using BlockType = pw::allocator::Block<>;
  using Range = typename BlockType::Range;

  /// Constexpr constructor. Callers must explicitly call `Init`.
  constexpr SimpleAllocator() = default;

  /// Initialize this allocator to allocate memory from `region`.
  Status Init(ByteSpan region) {
    auto result = BlockType::Init(region);
    if (result.ok()) {
      blocks_ = *result;
    }
    return result.status();
  }

  /// Return the range of blocks for this allocator.
  Range blocks() { return Range(blocks_); }

  /// Resets the object to an initial state.
  void Reset() { blocks_ = nullptr; }

 private:
  /// @copydoc Allocator::Allocate
  void* DoAllocate(Layout layout) override {
    for (auto* block : Range(blocks_)) {
      if (BlockType::AllocFirst(block, layout.size(), layout.alignment())
              .ok()) {
        return block->UsableSpace();
      }
    }
    return nullptr;
  }

  /// @copydoc Allocator::Deallocate
  void DoDeallocate(void* ptr, Layout) override {
    if (ptr == nullptr) {
      return;
    }
    auto* bytes = static_cast<std::byte*>(ptr);
    BlockType* block = BlockType::FromUsableSpace(bytes);
    BlockType::Free(block);
  }

  /// @copydoc Allocator::Resize
  bool DoResize(void* ptr, Layout, size_t new_size) override {
    if (ptr == nullptr) {
      return false;
    }
    auto* bytes = static_cast<std::byte*>(ptr);
    BlockType* block = BlockType::FromUsableSpace(bytes);
    return BlockType::Resize(block, new_size).ok();
  }

  /// @copydoc Allocator::GetLayout
  Result<Layout> DoGetLayout(const void* ptr) const override {
    for (auto* block : Range(blocks_)) {
      if (block->UsableSpace() == ptr) {
        return block->GetLayout();
      }
    }
    return Status::NotFound();
  }

  /// @copydoc Allocator::Query
  Status DoQuery(const void* ptr, Layout) const override {
    for (auto* block : Range(blocks_)) {
      if (block->UsableSpace() == ptr) {
        return OkStatus();
      }
    }
    return Status::OutOfRange();
  }

  BlockType* blocks_ = nullptr;
};
// DOCSTAG: [pw_allocator_examples_simple_allocator]

}  // namespace pw::allocator
