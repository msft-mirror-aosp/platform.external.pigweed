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

// This file provides common utilities across all token types.
#pragma once

#include <inttypes.h>

// This character is used to mark the start of all tokenized messages. For
// consistency, it is recommended to always use $ if possible.
// If required, a different non-Base64 character may be used as a prefix.
//
// A string version of the character is required for format-string-literal
// concatenation.
#define PW_TOKENIZER_NESTED_PREFIX_STR "$"
#define PW_TOKENIZER_NESTED_PREFIX PW_TOKENIZER_NESTED_PREFIX_STR[0]

/// Format specifier for a token argument.
#define PW_TOKEN_FMT() PW_TOKENIZER_NESTED_PREFIX_STR "#%08" PRIx32
