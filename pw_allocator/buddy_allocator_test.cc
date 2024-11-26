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

#include "pw_allocator/buddy_allocator.h"

#include <array>
#include <cstddef>

#include "pw_unit_test/framework.h"

namespace {

// Test fixtures.

using BuddyAllocator = ::pw::allocator::BuddyAllocator<>;
using ::pw::allocator::Layout;

class BuddyAllocatorTest : public ::testing::Test {
 protected:
  static constexpr size_t kBufferSize = 0x400;
  std::array<std::byte, kBufferSize> buffer_;
};

// Unit tests.

TEST_F(BuddyAllocatorTest, ExplicitlyInit) {
  BuddyAllocator allocator;
  allocator.Init(buffer_);
}

TEST_F(BuddyAllocatorTest, AllocateSmall) {
  BuddyAllocator allocator(buffer_);
  void* ptr = allocator.Allocate(Layout(BuddyAllocator::kMinOuterSize / 2, 1));
  ASSERT_NE(ptr, nullptr);
  allocator.Deallocate(ptr);
}

TEST_F(BuddyAllocatorTest, AllocateAllBlocks) {
  BuddyAllocator allocator(buffer_);
  pw::Vector<void*, kBufferSize / BuddyAllocator::kMinOuterSize> ptrs;
  while (true) {
    void* ptr = allocator.Allocate(Layout(1, 1));
    if (ptr == nullptr) {
      break;
    }
    ptrs.push_back(ptr);
  }
  while (!ptrs.empty()) {
    allocator.Deallocate(ptrs.back());
    ptrs.pop_back();
  }
}

TEST_F(BuddyAllocatorTest, AllocateLarge) {
  BuddyAllocator allocator(buffer_);
  void* ptr = allocator.Allocate(Layout(48, 1));
  ASSERT_NE(ptr, nullptr);
  allocator.Deallocate(ptr);
}

TEST_F(BuddyAllocatorTest, AllocateExcessiveSize) {
  BuddyAllocator allocator(buffer_);
  void* ptr = allocator.Allocate(Layout(786, 1));
  EXPECT_EQ(ptr, nullptr);
}

TEST_F(BuddyAllocatorTest, AllocateExcessiveAlignment) {
  BuddyAllocator allocator(buffer_);
  void* ptr = allocator.Allocate(Layout(48, 32));
  EXPECT_EQ(ptr, nullptr);
}

}  // namespace
