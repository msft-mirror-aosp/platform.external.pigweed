// Copyright 2020 The Pigweed Authors
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

#include <stddef.h>
#include <stdint.h>

#include "pw_preprocessor/util.h"

PW_EXTERN_C_START

// This typedef is deprecated. Use uint32_t instead.
typedef uint32_t pw_tokenizer_Payload;

// This function is deprecated. For use with pw_log_tokenized, call
// pw_log_tokenized_HandleLog. For other uses, implement a pw_tokenizer macro
// that calls a custom handler.
void pw_tokenizer_HandleEncodedMessageWithPayload(
    pw_tokenizer_Payload payload,
    const uint8_t encoded_message[],
    size_t size_bytes);

PW_EXTERN_C_END