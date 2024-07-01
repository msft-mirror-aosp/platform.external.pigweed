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
"""Pigweed python build environment for bazel."""

load("@rules_python//python:defs.bzl", "py_test")
load("//pw_build:selects.bzl", "TARGET_COMPATIBLE_WITH_HOST_SELECT")

def pw_py_test(**kwargs):
    """Wrapper for py_test providing some defaults.

    Specifically, this wrapper,

    * Defaults to setting `target_compatible_with` to
      `select(TARGET_COMPATIBLE_WITH_HOST_SELECT)`

    Args:
      **kwargs: Passed to py_test.
    """

    # Python tests are always host only, but allow a user to override
    # the default value.
    if kwargs.get("target_compatible_with") == None:
        kwargs["target_compatible_with"] = select(TARGET_COMPATIBLE_WITH_HOST_SELECT)

    py_test(**kwargs)
