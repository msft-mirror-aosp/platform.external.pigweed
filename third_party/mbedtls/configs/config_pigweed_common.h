// Copyright 2021 The Pigweed Authors
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

// Some common configs for using mbedtls in Pigweed. These include disabling of
// file system, socket and linux/windows specific features. See
// include/mbedtls/config.h for a detail explanation of these configurations.

#pragma once

// No file system support.
#undef MBEDTLS_FS_IO
// No posix socket support
#undef MBEDTLS_NET_C
// This feature requires file system support.
#undef MBEDTLS_PSA_ITS_FILE_C
// The following two require MBEDTLS_PSA_ITS_FILE_C
#undef MBEDTLS_PSA_CRYPTO_C
#undef MBEDTLS_PSA_CRYPTO_STORAGE_C
// This feature only works on Unix/Windows
#undef MBEDTLS_TIMING_C
// Use a custom entropy generator
#define MBEDTLS_NO_PLATFORM_ENTROPY
// Error string support for debugging
#define MBEDTLS_ERROR_C
// This feature requires MBEDTLS_PSA_CRYPTO_C.
#undef MBEDTLS_LMS_C

// Disable a number of TLS related features as we are not maintaining the TLS
// support.
#undef MBEDTLS_KEY_EXCHANGE_ECDH_ECDSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDH_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_PSK_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_RSA_ENABLED
#undef MBEDTLS_KEY_EXCHANGE_ECDHE_ECDSA_ENABLED
#undef MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_EPHEMERAL_ENABLED
#undef MBEDTLS_SSL_TLS1_3_KEY_EXCHANGE_MODE_PSK_EPHEMERAL_ENABLED
#undef MBEDTLS_HAVE_TIME
#undef MBEDTLS_HAVE_TIME_DATE

#if defined(_PW_MBEDTLS_IS_HOST) && !_PW_MBEDTLS_IS_HOST
#define MBEDTLS_PLATFORM_MEMORY
#define MBEDTLS_PLATFORM_NO_STD_FUNCTIONS

#include <stdio.h>
#define MBEDTLS_PLATFORM_SNPRINTF_ALT
#define MBEDTLS_PLATFORM_STD_SNPRINTF snprintf
#define MBEDTLS_PLATFORM_VSNPRINTF_ALT
#define MBEDTLS_PLATFORM_STD_VSNPRINTF vsnprintf

#define MBEDTLS_PLATFORM_FPRINTF_MACRO(stream, format, ...) (0)
#define MBEDTLS_PLATFORM_PRINTF_MACRO(format, ...) (0)
#define MBEDTLS_PLATFORM_EXIT_MACRO(exit_code)
#endif  // defined(_PW_MBEDTLS_IS_HOST) && !_PW_MBEDTLS_IS_HOST

#include "mbedtls/check_config.h"
