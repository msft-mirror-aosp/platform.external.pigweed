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

#include "pw_tokenizer/internal/enum.h"
#include "pw_tokenizer/tokenize.h"

namespace pw::tokenizer {

/// @brief Tokenizes a given enumerator value. Used in the vase of a tokenizing
/// log backend.
/// @param value enumerator value
/// @return The 32-bit token (@cpp_type{pw_tokenizer_Token})
template <typename T>
constexpr auto EnumToToken(T value) {
  static_assert(std::is_enum_v<T>, "Must be an enum");
  static_assert(sizeof(T) <= sizeof(Token), "Must fit in a token");
  return static_cast<Token>(value);
}

/// Returns a string representation of a given enumerator value name.
/// @brief Returns a string representation of a given enumerator value name.
/// Used in the case of a non-tokenizing log backend.
/// @param value enumerator value
/// @return constexpr char array
template <typename T>
constexpr const char* EnumToString(T value) {
  return PwTokenizerEnumToString(value);
}

}  // namespace pw::tokenizer

/// Tokenizes the given values within an enumerator. All values of the
/// enumerator must be present to compile and have the enumerator be tokenized
/// successfully.
/// This macro should be in the same namespace as the enum declaration to use
/// the `pw::tokenizer::EnumToString` function and avoid compilation errors.
#define PW_TOKENIZE_ENUM(fully_qualified_name, ...) \
  PW_DELEGATE_BY_ARG_COUNT(                         \
      _PW_TOKENIZE_ENUM_, fully_qualified_name, __VA_ARGS__)
