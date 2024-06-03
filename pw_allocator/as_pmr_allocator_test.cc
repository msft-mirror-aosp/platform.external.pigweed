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

#include "pw_allocator/as_pmr_allocator.h"

#include <deque>
#include <forward_list>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "pw_allocator/testing.h"
#include "pw_unit_test/framework.h"

namespace {

// Test fixtures.

using ::pw::allocator::AsPmrAllocator;
using ::pw::allocator::test::AllocatorForTest;

struct Foo {
  uint32_t value;

  Foo(uint32_t value_) : value(value_) {}
};

bool operator==(const Foo& lhs, const Foo& rhs) {
  return lhs.value == rhs.value;
}

bool operator<(const Foo& lhs, const Foo& rhs) { return lhs.value < rhs.value; }

}  // namespace

template <>
struct std::hash<Foo> {
  size_t operator()(const Foo& foo) const {
    return std::hash<uint32_t>()(foo.value);
  }
};

namespace {

struct Bar {
  std::array<std::byte, 16> buffer;

  Bar(int value) { std::memset(buffer.data(), value, buffer.size()); }
};

template <typename T, typename = void>
struct has_emplace_front : std::false_type {};

template <typename T>
struct has_emplace_front<
    T,
    std::void_t<decltype(std::declval<T>().emplace_front())>>
    : std::true_type{};

template <typename T, typename = void>
struct has_emplace_back : std::false_type {};

template <typename T>
struct has_emplace_back<T,
                        std::void_t<decltype(std::declval<T>().emplace_back())>>
    : std::true_type{};

template <typename T, typename = void>
struct has_key_type : std::false_type {};

template <typename T>
struct has_key_type<T, std::void_t<typename T::key_type>> : std::true_type{};

template <typename T, typename = void>
struct has_mapped_type : std::false_type {};

template <typename T>
struct has_mapped_type<T, std::void_t<typename T::mapped_type>>
    : std::true_type{};

template <typename Container, size_t kCapacity = 256>
void TestPmrAllocator() {
  AllocatorForTest<kCapacity> underlying;
  auto& requested_bytes = underlying.metrics().requested_bytes;
  EXPECT_EQ(requested_bytes.value(), 0U);

  AsPmrAllocator allocator = underlying.as_pmr();
  {
    Container container(allocator);
    size_t size = 0;
    // Sequence containers.
    if constexpr (has_emplace_front<Container>::value) {
      container.emplace_front(1);
      container.emplace_front(2);
      size += sizeof(typename Container::value_type) * 2;
    }
    if constexpr (has_emplace_back<Container>::value) {
      container.emplace_back(3);
      container.emplace_back(4);
      size += sizeof(typename Container::value_type) * 2;
    }
    // Associative containers.
    if constexpr (has_mapped_type<Container>::value) {
      container.insert({1, 10});
      container.insert({2, 20});
      size += sizeof(typename Container::key_type) * 2;
      size += sizeof(typename Container::mapped_type) * 2;
    } else if constexpr (has_key_type<Container>::value) {
      container.insert(3);
      container.insert(4);
      size += sizeof(typename Container::key_type) * 2;
    }
    EXPECT_GE(requested_bytes.value(), size);
  }
  EXPECT_EQ(requested_bytes.value(), 0U);
}

// Unit tests.

TEST(AsPmrAllocatorTest, Vector) { TestPmrAllocator<std::pmr::vector<Foo>>(); }

TEST(AsPmrAllocatorTest, Deque) {
  // Some implementations preallocate a lot of memory.
  TestPmrAllocator<std::pmr::deque<Foo>, 8192>();
}

TEST(AsPmrAllocatorTest, ForwardList) {
  TestPmrAllocator<std::pmr::forward_list<Foo>>();
}

TEST(AsPmrAllocatorTest, List) { TestPmrAllocator<std::pmr::list<Foo>>(); }

TEST(AsPmrAllocatorTest, Set) { TestPmrAllocator<std::pmr::set<Foo>>(); }

TEST(AsPmrAllocatorTest, Map) { TestPmrAllocator<std::pmr::map<Foo, Bar>>(); }

TEST(AsPmrAllocatorTest, MultiSet) {
  TestPmrAllocator<std::pmr::multiset<Foo>>();
}

TEST(AsPmrAllocatorTest, MultiMap) {
  TestPmrAllocator<std::pmr::multimap<Foo, Bar>>();
}

TEST(AsPmrAllocatorTest, UnorderedSet) {
  // Some implementations preallocate a lot of memory.
  TestPmrAllocator<std::pmr::unordered_set<Foo>, 1024>();
}

TEST(AsPmrAllocatorTest, UnorderedMap) {
  // Some implementations preallocate a lot of memory.
  TestPmrAllocator<std::pmr::unordered_map<Foo, Bar>, 1024>();
}

TEST(AsPmrAllocatorTest, UnorderedMultiSet) {
  // Some implementations preallocate a lot of memory.
  TestPmrAllocator<std::pmr::unordered_multiset<Foo>, 1024>();
}

TEST(AsPmrAllocatorTest, UnorderedMultiMap) {
  // Some implementations preallocate a lot of memory.
  TestPmrAllocator<std::pmr::unordered_multimap<Foo, Bar>, 1024>();
}

}  // namespace
