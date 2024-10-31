# Copyright 2024 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Rust toolchain configuration"""

HOSTS = [
    {
        "cipd_arch": "arm64",
        "cpu": "aarch64",
        "dylib_ext": ".so",
        "os": "linux",
        "triple": "aarch64-unknown-linux-gnu",
    },
    {
        "cipd_arch": "amd64",
        "cpu": "x86_64",
        "dylib_ext": ".so",
        "os": "linux",
        "triple": "x86_64-unknown-linux-gnu",
    },
    {
        "cipd_arch": "arm64",
        "cpu": "aarch64",
        "dylib_ext": ".dylib",
        "os": "macos",
        "triple": "aarch64-apple-darwin",
    },
    {
        "cipd_arch": "amd64",
        "cpu": "x86_64",
        "dylib_ext": ".dylib",
        "os": "macos",
        "triple": "x86_64-apple-darwin",
    },
]

EXTRA_TARGETS = [
    {
        "cpu": "armv6-m",
        "triple": "thumbv6m-none-eabi",
    },
    {
        "cpu": "armv7-m",
        "triple": "thumbv7m-none-eabi",
    },
    {
        "cpu": "armv7e-m",
        "triple": "thumbv7m-none-eabi",
    },
    {
        "cpu": "armv8-m",
        "triple": "thumbv7m-none-eabi",  # TODO: https://pwbug.dev/352342797 - This should be some variant of ARMv8-M.
    },
]

CHANNELS = [
    {
        "extra_rustc_flags": ["-Dwarnings", "-Zmacro-backtrace"],
        "name": "nightly",
        "target_settings": ["@rules_rust//rust/toolchain/channel:nightly"],
    },
    {
        # In order to approximate a stable toolchain with our nightly one, we
        # disable experimental features with the exception of `proc_macro_span`
        # because the `proc-marcro2` automatically detects the toolchain
        # as nightly and dynamically uses this feature.
        "extra_rustc_flags": ["-Dwarnings", "-Zallow-features=proc_macro_span,rustc_attrs"],
        "name": "stable",
        "target_settings": ["@rules_rust//rust/toolchain/channel:stable"],
    },
]
