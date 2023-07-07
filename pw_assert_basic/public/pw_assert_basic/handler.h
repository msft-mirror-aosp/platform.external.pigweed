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

#include "pw_preprocessor/compiler.h"
#include "pw_preprocessor/util.h"

// This is needed for testing the basic crash handler.
#define PW_ASSERT_BASIC_DISABLE_NORETURN 0

#if PW_ASSERT_BASIC_DISABLE_NORETURN
#define PW_ASSERT_NORETURN
#else
#define PW_ASSERT_NORETURN PW_NO_RETURN
#endif

PW_EXTERN_C_START

// Application-defined assert failure handler for pw_assert_basic.
// file_name - may be nullptr if not available
// line_number - may be -1 if not available
// function_name - may be nullptr if not available
// format & varags - The assert reason can be built using the format string and
//     the varargs.
//
// Applications must define this function; it is not defined by pw_assert_basic.
void pw_assert_basic_HandleFailure(const char* file_name,
                                   int line_number,
                                   const char* function_name,
                                   const char* format,
                                   ...)
    PW_PRINTF_FORMAT(4, 5) PW_ASSERT_NORETURN;

PW_EXTERN_C_END
