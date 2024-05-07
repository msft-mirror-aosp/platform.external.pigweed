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

#include "pw_allocator/block_allocator_testing.h"

#include "pw_assert/check.h"
#include "pw_bytes/alignment.h"
#include "pw_status/status.h"

namespace pw::allocator::test {

// Test fixtures.

void BlockAllocatorTestBase::SetUp() { ptrs_.fill(nullptr); }

void BlockAllocatorTestBase::Store(size_t index, void* ptr) {
  ptrs_[index] = ptr;
}

void* BlockAllocatorTestBase::Fetch(size_t index) { return ptrs_[index]; }

void BlockAllocatorTestBase::UseMemory(void* ptr, size_t size) {
  std::memset(ptr, 0x5a, size);
}

// Unit tests.

void BlockAllocatorTestBase::GetCapacity() {
  Allocator& allocator = GetAllocator();
  StatusWithSize capacity = allocator.GetCapacity();
  EXPECT_EQ(capacity.status(), OkStatus());
  EXPECT_EQ(capacity.size(), kCapacity);
}

void BlockAllocatorTestBase::AllocateLarge() {
  Allocator& allocator = GetAllocator();
  constexpr Layout layout = Layout::Of<std::byte[kLargeInnerSize]>();
  Store(0, allocator.Allocate(layout));
  ASSERT_NE(Fetch(0), nullptr);
  ByteSpan bytes = GetBytes();
  EXPECT_GE(Fetch(0), bytes.data());
  EXPECT_LE(Fetch(0), bytes.data() + bytes.size());
  UseMemory(Fetch(0), layout.size());
}

void BlockAllocatorTestBase::AllocateSmall() {
  Allocator& allocator = GetAllocator();
  constexpr Layout layout = Layout::Of<std::byte[kSmallInnerSize]>();
  Store(0, allocator.Allocate(layout));
  ASSERT_NE(Fetch(0), nullptr);
  ByteSpan bytes = GetBytes();
  EXPECT_GE(Fetch(0), bytes.data());
  EXPECT_LE(Fetch(0), bytes.data() + bytes.size());
  UseMemory(Fetch(0), layout.size());
}

void BlockAllocatorTestBase::AllocateTooLarge() {
  Allocator& allocator = GetAllocator();
  Store(0, allocator.Allocate(Layout::Of<std::byte[kCapacity * 2]>()));
  EXPECT_EQ(Fetch(0), nullptr);
}

void BlockAllocatorTestBase::AllocateLargeAlignment() {
  Allocator& allocator = GetAllocator();
  constexpr size_t kAlignment = 64;
  Store(0, allocator.Allocate(Layout(kLargeInnerSize, kAlignment)));
  ASSERT_NE(Fetch(0), nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(Fetch(0)) % kAlignment, 0U);
  UseMemory(Fetch(0), kLargeInnerSize);

  Store(1, allocator.Allocate(Layout(kLargeInnerSize, kAlignment)));
  ASSERT_NE(Fetch(1), nullptr);
  EXPECT_EQ(reinterpret_cast<uintptr_t>(Fetch(1)) % kAlignment, 0U);
  UseMemory(Fetch(1), kLargeInnerSize);
}

void BlockAllocatorTestBase::AllocateAlignmentFailure() {
  // Allocate a two blocks with an unaligned region between them.
  constexpr size_t kAlignment = 128;
  ByteSpan bytes = GetBytes();
  auto addr = reinterpret_cast<uintptr_t>(bytes.data());
  uintptr_t outer_size =
      AlignUp(addr + kDefaultBlockOverhead, kAlignment) - addr + 1;
  Allocator& allocator = GetAllocator({
      {outer_size, 0},
      {kLargeOuterSize, Preallocation::kIndexFree},
      {Preallocation::kSizeRemaining, 2},
  });

  // The allocator should be unable to create an aligned region..
  Store(1, allocator.Allocate(Layout(kLargeInnerSize, kAlignment)));
  EXPECT_EQ(Fetch(1), nullptr);
}

void BlockAllocatorTestBase::DeallocateNull() {
  Allocator& allocator = GetAllocator();
  allocator.Deallocate(nullptr);
}

void BlockAllocatorTestBase::DeallocateShuffled() {
  Allocator& allocator = GetAllocator();
  constexpr Layout layout = Layout::Of<std::byte[kSmallInnerSize]>();
  for (size_t i = 0; i < kNumPtrs; ++i) {
    Store(i, allocator.Allocate(layout));
    if (Fetch(i) == nullptr) {
      break;
    }
  }

  // Mix up the order of allocations.
  for (size_t i = 0; i < kNumPtrs; ++i) {
    if (i % 2 == 0 && i + 1 < kNumPtrs) {
      void* tmp = Fetch(i);
      Store(i, Fetch(i + 1));
      Store(i + 1, tmp);
    }
    if (i % 3 == 0 && i + 2 < kNumPtrs) {
      void* tmp = Fetch(i);
      Store(i, Fetch(i + 2));
      Store(i + 2, tmp);
    }
  }

  // Deallocate everything.
  for (size_t i = 0; i < kNumPtrs; ++i) {
    allocator.Deallocate(Fetch(i));
    Store(i, nullptr);
  }
}

void BlockAllocatorTestBase::ResizeNull() {
  Allocator& allocator = GetAllocator();
  size_t new_size = 1;
  EXPECT_FALSE(allocator.Resize(nullptr, new_size));
}

void BlockAllocatorTestBase::ResizeLargeSame() {
  Allocator& allocator = GetAllocator({
      {kLargeOuterSize, 0},
      {kLargeOuterSize, 1},
  });
  size_t new_size = kLargeInnerSize;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kLargeInnerSize);
}

void BlockAllocatorTestBase::ResizeLargeSmaller() {
  Allocator& allocator = GetAllocator({
      {kLargeOuterSize, 0},
      {kLargeOuterSize, 1},
  });
  size_t new_size = kSmallInnerSize;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kSmallInnerSize);
}

void BlockAllocatorTestBase::ResizeLargeLarger() {
  Allocator& allocator = GetAllocator({
      {kLargeOuterSize, 0},
      {kLargeOuterSize, Preallocation::kIndexFree},
      {kSmallOuterSize, 2},
  });
  size_t new_size = kLargeInnerSize * 2;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kLargeInnerSize * 2);
}

void BlockAllocatorTestBase::ResizeLargeLargerFailure() {
  Allocator& allocator = GetAllocator({
      {kLargeOuterSize, 0},
      {kSmallOuterSize, 12},
  });
  // Memory after ptr is already allocated, so `Resize` should fail.
  size_t new_size = kLargeInnerSize * 2;
  EXPECT_FALSE(allocator.Resize(Fetch(0), new_size));
}

void BlockAllocatorTestBase::ResizeSmallSame() {
  Allocator& allocator = GetAllocator({
      {kSmallOuterSize, 0},
      {kSmallOuterSize, 1},
  });
  size_t new_size = kSmallInnerSize;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kSmallInnerSize);
}

void BlockAllocatorTestBase::ResizeSmallSmaller() {
  Allocator& allocator = GetAllocator({
      {kSmallOuterSize, 0},
      {kSmallOuterSize, 1},
  });
  size_t new_size = kSmallInnerSize / 2;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kSmallInnerSize / 2);
}

void BlockAllocatorTestBase::ResizeSmallLarger() {
  Allocator& allocator = GetAllocator({
      {kSmallOuterSize, 0},
      {kSmallOuterSize, Preallocation::kIndexFree},
      {kSmallOuterSize, 2},
  });
  size_t new_size = kSmallInnerSize * 2;
  ASSERT_TRUE(allocator.Resize(Fetch(0), new_size));
  UseMemory(Fetch(0), kSmallInnerSize * 2);
}

void BlockAllocatorTestBase::ResizeSmallLargerFailure() {
  Allocator& allocator = GetAllocator({
      {kSmallOuterSize, 0},
      {kSmallOuterSize, 1},
  });
  // Memory after ptr is already allocated, so `Resize` should fail.
  size_t new_size = kSmallInnerSize * 2 + kDefaultBlockOverhead;
  EXPECT_FALSE(allocator.Resize(Fetch(0), new_size));
}

void BlockAllocatorTestBase::CanGetLayoutFromValidPointer() {
  Allocator& allocator = GetAllocator();
  constexpr size_t kAlignment = 64;
  Store(0, allocator.Allocate(Layout(kLargeInnerSize, kAlignment * 2)));
  ASSERT_NE(Fetch(0), nullptr);

  Store(1, allocator.Allocate(Layout(kSmallInnerSize, kAlignment / 2)));
  ASSERT_NE(Fetch(1), nullptr);

  Result<Layout> result0 = allocator.GetLayout(Fetch(0));
  ASSERT_EQ(result0.status(), OkStatus());
  EXPECT_GE(result0->size(), kLargeInnerSize);
  EXPECT_EQ(result0->alignment(), kAlignment * 2);

  Result<Layout> result1 = allocator.GetLayout(Fetch(1));
  ASSERT_EQ(result1.status(), OkStatus());
  EXPECT_GE(result1->size(), kSmallInnerSize);
  EXPECT_EQ(result1->alignment(), kAlignment / 2);
}

}  // namespace pw::allocator::test
