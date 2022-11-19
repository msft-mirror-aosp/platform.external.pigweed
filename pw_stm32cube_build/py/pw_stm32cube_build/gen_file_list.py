# Copyright 2021 The Pigweed Authors
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
"""Generates a list of relevant files present in a stm32cube source package."""

from typing import List

import pathlib


def gen_file_list(stm32cube_dir: pathlib.Path):
    """Generates `files.txt` for stm32cube directories

    The paths in `files.txt` are relative paths to the files in the 'posix'
    path format.

    Args:
        stm32cube_dir: stm32cube directory containing 'hal_driver',
            'cmsis_core' and 'cmsis_device

    Raises
        AssertionError if the provided directory is invalid
    """

    assert (stm32cube_dir / 'hal_driver').is_dir(), 'hal_driver not found'
    assert (stm32cube_dir / 'cmsis_core').is_dir(), 'cmsis_core not found'
    assert (stm32cube_dir / 'cmsis_device').is_dir(), 'cmsis_device not found'

    file_paths: List[pathlib.Path] = []
    file_paths.extend(stm32cube_dir.glob("**/*.h"))
    file_paths.extend(stm32cube_dir.glob("**/*.c"))
    file_paths.extend(stm32cube_dir.glob("**/*.s"))
    file_paths.extend(stm32cube_dir.glob("**/*.ld"))
    file_paths.extend(stm32cube_dir.glob("**/*.icf"))

    #TODO(vars): allow arbitrary path for generated file list
    with open(stm32cube_dir / "files.txt", "w") as out_file:
        out_file.write('# Generated by pw_stm32cube_build/gen_file_list\n')
        for file_path in file_paths:
            out_file.write(
                file_path.relative_to(stm32cube_dir).as_posix() + '\n')
