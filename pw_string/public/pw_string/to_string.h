// Copyright 2019 The Pigweed Authors
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

// Provides the ToString function, which outputs string representations of
// arbitrary types to a buffer.
//
// ToString returns the number of characters written, excluding the null
// terminator, and a status. A null terminator is always written if the output
// buffer has room.
//
// ToString functions may be defined for any type. This is done by providing a
// ToString template specialization in the pw namespace. The specialization must
// follow ToString's semantics:
//
//   1. Always null terminate if the output buffer has room.
//   2. Return the number of characters written, excluding the null terminator,
//      as a StatusWithSize.
//   3. If the buffer is too small to fit the output, return a StatusWithSize
//      with the number of characters written and a status of
//      RESOURCE_EXHAUSTED. Other status codes may be used for different errors.
//
// For example, providing the following specialization would allow ToString, and
// any classes that use it, to print instances of a custom type:
//
//   namespace pw {
//
//   template <>
//   StatusWithSize ToString<SomeCustomType>(const SomeCustomType& value,
//                           span<char> buffer) {
//     return /* ... implementation ... */;
//   }
//
//   }  // namespace pw
//
// Note that none of the functions in this module use std::snprintf. ToString
// overloads may use snprintf if needed, but the ToString semantics must be
// maintained.
//
// ToString is a low-level function. To write complex objects to string, a
// StringBuilder may be easier to work with. StringBuilder's operator<< may be
// overloaded for custom types.

#include <chrono>
#include <optional>
#include <string_view>
#include <type_traits>

#include "pw_string/to_string_basic.h"

namespace pw {

namespace internal {

/// A shared duration type represented using int64_t nanoseconds.
using UnifiedDuration = std::chrono::duration<int64_t, std::nano>;

// Whether or not an std::chrono::duration<Rep, Period> type can be converted
// to `std::chrono::duration<int64_t, std::nano>`.
template <typename T, typename = void>
constexpr bool is_unifyable_duration_v = false;

template <typename T>
constexpr bool is_unifyable_duration_v<
    T,
    std::void_t<decltype(std::chrono::round<UnifiedDuration>(
        std::declval<T>()))>> = true;

template <typename Rep,
          typename Period,
          typename = std::enable_if_t<internal::is_unifyable_duration_v<
              std::chrono::duration<Rep, Period>>>>
inline StatusWithSize DurationToString(
    const std::chrono::duration<Rep, Period>& duration, span<char> buffer) {
  // Cast to SourceDuration type before comparison, as naive comparison
  // may result in casts the other direction or from float -> int which
  // cause undefined behavior.
  using SourceDuration = std::chrono::duration<Rep, Period>;

  // std::chrono::years is not available until C++20.
  using Years = std::chrono::duration<int64_t, std::ratio<31556952>>;

  // std::chrono specifies support for at least +/-292 years. Beyond that,
  // the chrono and duration APIs do not take care to avoid overflow, so
  // we must short-circuit.
  if (duration < std::chrono::ceil<SourceDuration>(Years(-292))) {
    return ToString("An unrepresentably long time ago (>292 years).", buffer);
  }
  if (duration > std::chrono::floor<SourceDuration>(Years(292))) {
    return ToString("An unrepresentably long time in the future (>292 years).",
                    buffer);
  }

  // This is an approximation which unfortunately can result in some errors
  // reporting things like `21321839210 != 21321839210` (both values different,
  // but rounded to the same). Unfortunately there isn't an "easy" alternative:
  // duration_cast here can result in UB (even with only integer reps!) due to
  // an unchecked multiplication by the ratio's numerator before the division
  // by the denominator.
  auto nanos = std::chrono::round<UnifiedDuration>(duration);
  StatusWithSize s;
  s.UpdateAndAdd(ToString(nanos.count(), buffer));
  s.UpdateAndAdd(ToString("ns", buffer.subspan(s.size())));
  s.ZeroIfNotOk();
  return s;
}

}  // namespace internal

/// @submodule{pw_string,util}

// ToString overloads for std::chrono types.
template <
    typename Clock,
    typename Duration,
    typename = std::enable_if_t<internal::is_unifyable_duration_v<Duration>>>
inline StatusWithSize ToString(
    const std::chrono::time_point<Clock, Duration>& time_point,
    span<char> buffer) {
  return internal::DurationToString(time_point.time_since_epoch(), buffer);
}

template <typename Rep,
          typename Period,
          typename = std::enable_if_t<internal::is_unifyable_duration_v<
              std::chrono::duration<Rep, Period>>>>
inline StatusWithSize ToString(
    const std::chrono::duration<Rep, Period>& duration, span<char> buffer) {
  return internal::DurationToString(duration, buffer);
}

/// @}

}  // namespace pw
