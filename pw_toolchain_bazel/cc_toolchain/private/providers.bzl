# Copyright 2023 The Pigweed Authors
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
"""All shared providers that act as an API between toolchain-related rules."""

ToolchainFeatureInfo = provider(
    doc = "A provider containing cc_toolchain features and related fields.",
    fields = {
        "feature": "feature: The a group of build flags structured as a toolchain feature",
        "cxx_builtin_include_directories": "List[str]: Builtin C/C++ standard library include directories",
    },
)
