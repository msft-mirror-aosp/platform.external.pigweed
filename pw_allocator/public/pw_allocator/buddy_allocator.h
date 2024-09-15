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

#include <array>
#include <cstddef>

#include "pw_alignment/alignment.h"
#include "pw_allocator/allocator.h"
#include "pw_allocator/bucket.h"
#include "pw_bytes/span.h"
#include "pw_containers/vector.h"
#include "pw_status/try.h"

namespace pw::allocator {
namespace internal {

/// Size-independent buddy allocator.
///
/// This allocator allocates chunks of memory whose sizes are powers of two.
/// See also https://en.wikipedia.org/wiki/Buddy_memory_allocation.
///
/// Compared to `BuddyAllocator`, this implementation is size-agnostic with
/// respect to the number of buckets.
class GenericBuddyAllocator final {
 public:
  static constexpr Capabilities kCapabilities =
      kImplementsGetUsableLayout | kImplementsGetAllocatedLayout |
      kImplementsGetCapacity | kImplementsRecognizes;

  /// Constructs a buddy allocator.
  ///
  /// @param[in] buckets        Storage for buckets of free chunks.
  /// @param[in] min_chunk_size Size of the chunks in the first bucket.
  GenericBuddyAllocator(span<Bucket> buckets, size_t min_chunk_size);

  /// Sets the memory used to allocate chunks.
  void Init(ByteSpan region);

  /// @copydoc Allocator::Allocate
  void* Allocate(Layout layout);

  /// @copydoc Deallocator::Deallocate
  void Deallocate(void* ptr);

  /// Returns the total capacity of this allocator.
  size_t GetCapacity() const { return region_.size(); }

  /// Returns the allocated layout for a given pointer.
  Result<Layout> GetLayout(const void* ptr) const;

  /// Ensures all allocations have been freed. Crashes with a diagnostic message
  /// If any allocations remain outstanding.
  void CrashIfAllocated();

 private:
  span<Bucket> buckets_;
  ByteSpan region_;
};

}  // namespace internal

/// Allocator that uses the buddy memory allocation algorithm.
///
/// This allocator allocates chunks of memory whose sizes are powers of two.
/// This allows the allocator to satisfy requests to acquire and release memory
/// very quickly, at the possible cost of higher internal fragmentation. In
/// particular:
///
/// * The maximum alignment for this allocator is `kMinChunkSize`.
/// * The minimum size of an allocation is `kMinChunkSize`. Less may be
///   requested, but it will be satisfied by a minimal chunk.
/// * The maximum size of an allocation is `kMinChunkSize << (kNumBuckets - 1)`.
///
/// Use this allocator if you know the needed sizes are close to but less than
/// chunk sizes and you need high allocator performance.
///
/// @tparam   kMinChunkSize   Size of the smallest allocatable chunk. Must be a
///                           a power of two. All allocations will use at least
///                           this much memory.
/// @tparam   kNumBuckets     Number of buckets. Must be at least 1. Each
///                           additional bucket allows combining chunks into
///                           larger chunks.
template <size_t kMinChunkSize, size_t kNumBuckets>
class BuddyAllocator : public Allocator {
 public:
  static_assert((kMinChunkSize & (kMinChunkSize - 1)) == 0,
                "kMinChunkSize must be a power of 2");

  /// Constructs an allocator. Callers must call `Init`.
  BuddyAllocator() : impl_(buckets_, kMinChunkSize) {}

  /// Constructs an allocator, and initializes it with the given memory region.
  ///
  /// @param[in]  region  Region of memory to use when satisfying allocation
  ///                     requests. The region MUST be large enough to fit a
  ///                     least one minimally-size chunk aligned to the size of
  ///                     the chunk.
  BuddyAllocator(ByteSpan region) : BuddyAllocator() { Init(region); }

  /// Sets the memory region used by the allocator.
  ///
  /// @param[in]  region  Region of memory to use when satisfying allocation
  ///                     requests. The region MUST be large enough to fit a
  ///                     least one minimally-size chunk aligned to the size of
  ///                     the chunk.
  void Init(ByteSpan region) { impl_.Init(region); }

  ~BuddyAllocator() override { impl_.CrashIfAllocated(); }

 private:
  /// @copydoc Allocator::Allocate
  void* DoAllocate(Layout layout) override {
    // Reserve one byte to save the bucket index.
    return impl_.Allocate(layout.Extend(1));
  }

  /// @copydoc Deallocator::DoDeallocate
  void DoDeallocate(void* ptr) override { impl_.Deallocate(ptr); }

  /// @copydoc Deallocator::GetInfo
  Result<Layout> DoGetInfo(InfoType info_type, const void* ptr) const override {
    switch (info_type) {
      case InfoType::kUsableLayoutOf: {
        Layout layout;
        PW_TRY_ASSIGN(layout, impl_.GetLayout(ptr));
        return Layout(layout.size() - 1, layout.alignment());
      }
      case InfoType::kAllocatedLayoutOf:
        return impl_.GetLayout(ptr);
      case InfoType::kCapacity:
        return Layout(impl_.GetCapacity(), kMinChunkSize);
      case InfoType::kRecognizes: {
        Layout layout;
        PW_TRY_ASSIGN(layout, impl_.GetLayout(ptr));
        return Layout();
      }
      case InfoType::kRequestedLayoutOf:
      default:
        return Status::Unimplemented();
    }
  }

  std::array<internal::Bucket, kNumBuckets> buckets_;
  internal::GenericBuddyAllocator impl_;
};

}  // namespace pw::allocator
