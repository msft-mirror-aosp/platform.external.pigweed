#!/usr/bin/env python3
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
"""Flashes binaries to attached Raspberry Pi Pico boards."""

import logging
import os
from pathlib import Path
import subprocess
import time

import serial  # type: ignore

from rp2040_utils import device_detector
from rp2040_utils.device_detector import PicoBoardInfo, PicoDebugProbeBoardInfo

_LOG = logging.getLogger()

# If the script is being run through Bazel, our support binaries are provided
# at well known locations in its runfiles.
try:
    from rules_python.python.runfiles import runfiles  # type: ignore

    r = runfiles.Create()
    _PROBE_RS_COMMAND = r.Rlocation('pigweed/third_party/probe-rs/probe-rs')
    _PICOTOOL_COMMAND = r.Rlocation('picotool/picotool')
except ImportError:
    _PROBE_RS_COMMAND = 'probe-rs'
    _PICOTOOL_COMMAND = 'picotool'


def flash(board_info: PicoBoardInfo, baud_rate: int, binary: Path) -> bool:
    """Load `binary` onto `board_info` and wait for the device to become
    available.

    Returns whether or not flashing was successful."""
    if isinstance(board_info, PicoDebugProbeBoardInfo):
        return _load_debugprobe_binary(board_info, binary)
    if not _load_picotool_binary(board_info, binary):
        return False
    if not _wait_for_serial_port(board_info, baud_rate):
        _LOG.error(
            'Binary flashed but unable to connect to the serial port: %s',
            board_info.serial_port,
        )
        return False
    return True


def find_elf(binary: Path) -> Path | None:
    """Attempt to find and return the path to an ELF file for a binary.

    Args:
      binary: A relative path to a binary.

    Returns the path to the associated ELF file, or None if none was found.
    """
    if binary.suffix == '.elf' or not binary.suffix:
        return binary
    choices = (
        binary.parent / f'{binary.stem}.elf',
        binary.parent / 'bin' / f'{binary.stem}.elf',
        binary.parent / 'test' / f'{binary.stem}.elf',
    )
    for choice in choices:
        if choice.is_file():
            return choice

    _LOG.error(
        'Cannot find ELF file to use as a token database for binary: %s',
        binary,
    )
    return None


def _load_picotool_binary(board_info: PicoBoardInfo, binary: Path) -> bool:
    """Flash a binary to this device using picotool, returning success or
    failure."""
    cmd = [
        _PICOTOOL_COMMAND,
        'load',
        '-x',
        str(binary),
    ]

    # If the binary has not file extension, assume that it is ELF and
    # explicitly tell `picotool` that.
    if not binary.suffix:
        cmd += ['-t', 'elf']

    cmd += [
        '--bus',
        str(board_info.bus),
        '--address',
        str(board_info.address()),
        '-F',
    ]

    _LOG.debug('Flashing ==> %s', ' '.join(cmd))

    # If the script is running inside Bazel, `picotool` needs to run from
    # the project root so that it can find libusb.
    cwd = None
    if 'BUILD_WORKING_DIRECTORY' in os.environ:
        cwd = os.environ['BUILD_WORKING_DIRECTORY']

    process = subprocess.run(
        cmd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=cwd,
    )
    if process.returncode:
        err = (
            'Flashing command failed: ' + ' '.join(cmd),
            str(board_info),
            process.stdout.decode('utf-8', errors='replace'),
        )
        _LOG.error('\n\n'.join(err))
        return False
    return True


def _wait_for_serial_port(board_info: PicoBoardInfo, baud_rate: int) -> bool:
    """Waits for a serial port to enumerate."""
    start_time = time.monotonic()
    timeout_seconds = 10.0
    while time.monotonic() - start_time < timeout_seconds:
        # If the serial port path isn't known, watch for a newly
        # enumerated path.
        if board_info.serial_port is None:
            # Wait a bit before checking for a new port.
            time.sleep(0.3)
            # Check for updated serial port path.
            for device in device_detector.detect_boards():
                if (
                    device.bus == board_info.bus
                    and device.port == board_info.port
                ):
                    board_info.serial_port = device.serial_port
                    # Serial port found, break out of device for loop.
                    break

        # Serial port known try to connect to it.
        if board_info.serial_port is not None:
            # Connect to the serial port.
            try:
                serial.Serial(baudrate=baud_rate, port=board_info.serial_port)
                return True
            except serial.serialutil.SerialException:
                # Unable to connect, try again.
                _LOG.debug(
                    'Unable to connect to %s, retrying', board_info.serial_port
                )
                time.sleep(0.1)

    _LOG.error(
        'Binary flashed but unable to connect to the serial port: %s',
        board_info.serial_port,
    )
    return False


def _load_debugprobe_binary(
    board_info: PicoDebugProbeBoardInfo, binary: Path
) -> bool:
    """Flash a binary to this device using a debug probe, returning success
    or failure."""
    elf_path = find_elf(binary)
    if not elf_path:
        return False

    # `probe-rs` takes a `--probe` argument of the form:
    #  <vendor_id>:<product_id>:<serial_number>
    probe = "{:04x}:{:04x}:{}".format(
        board_info.vendor_id(),
        board_info.device_id(),
        board_info.serial_number,
    )

    download_cmd = (
        _PROBE_RS_COMMAND,
        'download',
        '--probe',
        probe,
        '--chip',
        'RP2040',
        '--speed',
        '10000',
        str(elf_path),
    )
    _LOG.debug('Flashing ==> %s', ' '.join(download_cmd))
    process = subprocess.run(
        download_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    if process.returncode:
        err = (
            'Flashing command failed: ' + ' '.join(download_cmd),
            str(board_info),
            process.stdout.decode('utf-8', errors='replace'),
        )
        _LOG.error('\n\n'.join(err))
        return False

    # `probe-rs download` leaves the device halted so it needs to be reset
    # to run.
    reset_cmd = (
        _PROBE_RS_COMMAND,
        'reset',
        '--probe',
        probe,
        '--chip',
        'RP2040',
    )
    _LOG.debug('Resetting ==> %s', ' '.join(reset_cmd))
    process = subprocess.run(
        reset_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
    )
    if process.returncode:
        err = (
            'Resetting command failed: ' + ' '.join(reset_cmd),
            str(board_info),
            process.stdout.decode('utf-8', errors='replace'),
        )
        _LOG.error('\n\n'.join(err))
        return False

    # Give time for the device to reset. Ideally the common unit test
    # runner would wait for input but this is not the case.
    time.sleep(0.5)

    return True
