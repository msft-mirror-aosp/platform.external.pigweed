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

#include "pw_allocator/multiplex_allocator.h"

#include "pw_allocator/allocator_testing.h"
#include "pw_tokenizer/tokenize.h"
#include "pw_unit_test/framework.h"

namespace pw::allocator {
namespace {

// Test fixtures.

constexpr size_t kHeapSize = 256;

constexpr tokenizer::Token kMultiplexToken = PW_TOKENIZE_STRING("multiplexed");

// Token and layout as might be defined by an application.
constexpr tokenizer::Token kFooToken = PW_TOKENIZE_STRING("foo");
constexpr Layout kFooLayout(32, 16);

// Token and layout distinct from those above.
constexpr tokenizer::Token kBarToken = PW_TOKENIZE_STRING("bar");
constexpr Layout kBarLayout(128, 4);

// Token that will be mapped to the same allocator as `kBarToken`.
constexpr tokenizer::Token kBazToken = PW_TOKENIZE_STRING("baz");

// Token that will be explicitly mapped to nothing.
constexpr tokenizer::Token kQuxToken = PW_TOKENIZE_STRING("qux");

// Token that isn't recognized by the multiplex allocator.
constexpr tokenizer::Token kInvalidToken = PW_TOKENIZE_STRING("invalid");

/// Types to use for suballocators.
using Suballocator = test::AllocatorForTest<kHeapSize>;

/// Helper method to restore suballocator type, in order to make test methods
/// easier to access.
template <typename MultiplexAllocatorType>
Suballocator::allocator_type* GetSuballocator(MultiplexAllocatorType& allocator,
                                              tokenizer::Token token) {
  return static_cast<Suballocator::allocator_type*>(
      allocator.GetAllocator(token));
}

/// Represents a MultiplexAllocator with a simple mapping.
using MultiplexAllocatorForTest =
    FlatMapMultiplexAllocatorImpl<internal::Metrics, 4>;

/// Represents a MultiplexAllocator with custom logic.
class CustomMultiplexAllocatorForTest
    : public MultiplexAllocatorImpl<internal::Metrics> {
 public:
  using Base = MultiplexAllocatorImpl<internal::Metrics>;
  using allocator_type = typename Base::allocator_type;

  CustomMultiplexAllocatorForTest(allocator_type* foo, allocator_type* bar)
      : Base(kMultiplexToken), foo_(foo), bar_(bar) {
    AddMetrics(foo_);
    AddMetrics(bar_);
  }

 private:
  allocator_type* DoGetAllocator(Token token) override {
    switch (token) {
      case kFooToken:
        return foo_;
      case kBarToken:
      case kBazToken:
        return bar_;
      case kQuxToken:
      default:
        return nullptr;
    }
  }

  allocator_type* foo_;
  allocator_type* bar_;
};

// Unit tests

template <
    typename MultiplexAllocatorType,
    typename AllocatorType = typename MultiplexAllocatorType::allocator_type>
void GetAllocator(MultiplexAllocatorType& allocator,
                  AllocatorType* foo,
                  AllocatorType* bar) {
  EXPECT_EQ(allocator.GetAllocator(kFooToken), foo);
  EXPECT_EQ(allocator.GetAllocator(kBarToken), bar);
  EXPECT_EQ(allocator.GetAllocator(kBazToken), bar);
  EXPECT_EQ(allocator.GetAllocator(kQuxToken), nullptr);
  EXPECT_EQ(allocator.GetAllocator(kInvalidToken), nullptr);
}
TEST(MultiplexAllocatorTest, GetAllocator) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  GetAllocator(allocator, foo.get(), bar.get());
}
TEST(MultiplexAllocatorTest, GetAllocatorCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  GetAllocator(allocator, foo.get(), bar.get());
}

template <typename MultiplexAllocatorType>
void AllocateValidToken(MultiplexAllocatorType& allocator) {
  EXPECT_NE(allocator.Allocate(kFooToken, kFooLayout), nullptr);
  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->allocate_size(), kFooLayout.size());

  EXPECT_NE(allocator.Allocate(kBarToken, kBarLayout), nullptr);
  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->allocate_size(), kBarLayout.size());
}
TEST(MultiplexAllocatorTest, AllocateValidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  AllocateValidToken(allocator);
}
TEST(MultiplexAllocatorTest, AllocateValidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  AllocateValidToken(allocator);
}

template <typename MultiplexAllocatorType>
void AllocateInvalidToken(MultiplexAllocatorType& allocator) {
  EXPECT_EQ(allocator.Allocate(kQuxToken, kFooLayout), nullptr);
  EXPECT_EQ(allocator.Allocate(kInvalidToken, kBarLayout), nullptr);

  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->allocate_size(), 0U);

  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->allocate_size(), 0U);
}
TEST(MultiplexAllocatorTest, AllocateInvalidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  AllocateInvalidToken(allocator);
}
TEST(MultiplexAllocatorTest, AllocateInvalidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  AllocateInvalidToken(allocator);
}

template <typename MultiplexAllocatorType>
void DeallocateValidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);

  allocator.Deallocate(kFooToken, foo_ptr, kFooLayout);
  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->deallocate_ptr(), foo_ptr);
  EXPECT_EQ(foo->deallocate_size(), kFooLayout.size());

  allocator.Deallocate(kBarToken, bar_ptr, kBarLayout);
  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->deallocate_ptr(), bar_ptr);
  EXPECT_EQ(bar->deallocate_size(), kBarLayout.size());
}
TEST(MultiplexAllocatorTest, DeallocateValidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  DeallocateValidToken(allocator);
}
TEST(MultiplexAllocatorTest, DeallocateValidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  DeallocateValidToken(allocator);
}

template <typename MultiplexAllocatorType>
void DeallocateInvalidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);
  allocator.Deallocate(kQuxToken, foo_ptr, kFooLayout);
  allocator.Deallocate(kInvalidToken, bar_ptr, kBarLayout);

  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->deallocate_ptr(), nullptr);
  EXPECT_EQ(foo->deallocate_size(), 0U);

  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->deallocate_ptr(), nullptr);
  EXPECT_EQ(bar->deallocate_size(), 0U);
}
TEST(MultiplexAllocatorTest, DeallocateInvalidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  DeallocateInvalidToken(allocator);
}
TEST(MultiplexAllocatorTest, DeallocateInvalidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  DeallocateInvalidToken(allocator);
}

template <typename MultiplexAllocatorType>
void ResizeValidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);

  EXPECT_TRUE(
      allocator.Resize(kFooToken, foo_ptr, kFooLayout, kFooLayout.size() * 2));
  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->resize_ptr(), foo_ptr);
  EXPECT_EQ(foo->resize_old_size(), kFooLayout.size());
  EXPECT_EQ(foo->resize_new_size(), kFooLayout.size() * 2);

  EXPECT_TRUE(
      allocator.Resize(kBarToken, bar_ptr, kBarLayout, kBarLayout.size() / 2));
  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->resize_ptr(), bar_ptr);
  EXPECT_EQ(bar->resize_old_size(), kBarLayout.size());
  EXPECT_EQ(bar->resize_new_size(), kBarLayout.size() / 2);
}
TEST(MultiplexAllocatorTest, ResizeValidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  ResizeValidToken(allocator);
}
TEST(MultiplexAllocatorTest, ResizeValidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  ResizeValidToken(allocator);
}

template <typename MultiplexAllocatorType>
void ResizeInvalidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);
  EXPECT_FALSE(
      allocator.Resize(kQuxToken, foo_ptr, kFooLayout, kFooLayout.size() / 2));
  EXPECT_FALSE(allocator.Resize(
      kInvalidToken, bar_ptr, kBarLayout, kBarLayout.size() / 2));

  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->resize_ptr(), nullptr);
  EXPECT_EQ(foo->resize_old_size(), 0U);
  EXPECT_EQ(foo->resize_new_size(), 0U);

  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->resize_ptr(), nullptr);
  EXPECT_EQ(bar->resize_old_size(), 0U);
  EXPECT_EQ(bar->resize_new_size(), 0U);
}
TEST(MultiplexAllocatorTest, ResizeInvalidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  ResizeInvalidToken(allocator);
}
TEST(MultiplexAllocatorTest, ResizeInvalidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  ResizeInvalidToken(allocator);
}

template <typename MultiplexAllocatorType>
void ReallocateValidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);

  EXPECT_NE(allocator.Reallocate(
                kFooToken, foo_ptr, kFooLayout, kFooLayout.size() * 2),
            nullptr);
  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->resize_ptr(), foo_ptr);
  EXPECT_EQ(foo->resize_old_size(), kFooLayout.size());
  EXPECT_EQ(foo->resize_new_size(), kFooLayout.size() * 2);

  EXPECT_NE(allocator.Reallocate(
                kBarToken, bar_ptr, kBarLayout, kBarLayout.size() / 2),
            nullptr);
  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->resize_ptr(), bar_ptr);
  EXPECT_EQ(bar->resize_old_size(), kBarLayout.size());
  EXPECT_EQ(bar->resize_new_size(), kBarLayout.size() / 2);
}
TEST(MultiplexAllocatorTest, ReallocateValidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  ReallocateValidToken(allocator);
}
TEST(MultiplexAllocatorTest, ReallocateValidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  ReallocateValidToken(allocator);
}

template <typename MultiplexAllocatorType>
void ReallocateInvalidToken(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  auto* foo = GetSuballocator(allocator, kFooToken);
  foo->ResetParameters();

  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);
  auto* bar = GetSuballocator(allocator, kBarToken);
  bar->ResetParameters();

  EXPECT_EQ(allocator.Reallocate(
                kQuxToken, foo_ptr, kFooLayout, kFooLayout.size() * 2),
            nullptr);
  EXPECT_EQ(allocator.Reallocate(
                kInvalidToken, bar_ptr, kBarLayout, kBarLayout.size() / 2),
            nullptr);

  EXPECT_EQ(foo->allocate_size(), 0U);
  EXPECT_EQ(foo->resize_ptr(), nullptr);
  EXPECT_EQ(foo->resize_old_size(), 0U);
  EXPECT_EQ(foo->resize_new_size(), 0U);

  EXPECT_EQ(bar->allocate_size(), 0U);
  EXPECT_EQ(bar->resize_ptr(), nullptr);
  EXPECT_EQ(bar->resize_old_size(), 0U);
  EXPECT_EQ(bar->resize_new_size(), 0U);
}
TEST(MultiplexAllocatorTest, ReallocateInvalidToken) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  ReallocateInvalidToken(allocator);
}
TEST(MultiplexAllocatorTest, ReallocateInvalidTokenCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  ReallocateInvalidToken(allocator);
}

template <typename MultiplexAllocatorType>
void UpdateMetrics(MultiplexAllocatorType& allocator) {
  void* foo_ptr = allocator.Allocate(kFooToken, kFooLayout);
  void* bar_ptr = allocator.Allocate(kBarToken, kBarLayout);
  EXPECT_TRUE(
      allocator.Resize(kFooToken, foo_ptr, kFooLayout, kFooLayout.size() * 2));
  EXPECT_TRUE(
      allocator.Resize(kBarToken, bar_ptr, kBarLayout, kBarLayout.size() / 2));

  auto* foo = GetSuballocator(allocator, kFooToken);
  EXPECT_EQ(foo->used(), kFooLayout.size() * 2);
  EXPECT_EQ(foo->peak(), kFooLayout.size() * 2);
  EXPECT_EQ(foo->count(), 1U);

  auto* bar = GetSuballocator(allocator, kBarToken);
  EXPECT_EQ(bar->used(), kBarLayout.size() / 2);
  EXPECT_EQ(bar->peak(), kBarLayout.size());
  EXPECT_EQ(bar->count(), 1U);

  EXPECT_EQ(allocator.used(), foo->used() + bar->used());
  EXPECT_EQ(allocator.peak(), foo->peak() + bar->peak());
  EXPECT_EQ(allocator.count(), foo->count() + bar->count());
}
TEST(MultiplexAllocatorTest, UpdateMetrics) {
  Suballocator foo, bar;
  MultiplexAllocatorForTest allocator(kMultiplexToken,
                                      {{{kFooToken, foo.get()},
                                        {kBarToken, bar.get()},
                                        {kBazToken, bar.get()},
                                        {kQuxToken, nullptr}}});
  UpdateMetrics(allocator);
}
TEST(MultiplexAllocatorTest, UpdateMetricsCustom) {
  Suballocator foo, bar;
  CustomMultiplexAllocatorForTest allocator(foo.get(), bar.get());
  UpdateMetrics(allocator);
}

}  // namespace
}  // namespace pw::allocator
