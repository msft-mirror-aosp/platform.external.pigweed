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
"""Provides utilities for registering custom command alias."""

import subprocess
import sys

from typing import Callable


def alias(*command) -> Callable[[], int]:
    """Creates a *command alias, conctenated with CLI arguments in runtime."""
    # Exclude the first argv: `subcmd arg1 arg2` --> `arg1 arg2`.
    return lambda: subprocess.run([*command, *sys.argv[1:]]).returncode
