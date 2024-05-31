#!/usr/bin/env python3
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
"""This script flashes and runs unit tests on Raspberry Pi Pico boards."""

import argparse
import logging
import subprocess
import sys
import time
from pathlib import Path

import serial  # type: ignore

import pw_cli.log
from pw_unit_test import serial_test_runner
from pw_unit_test.serial_test_runner import (
    SerialTestingDevice,
    DeviceNotFound,
)
from rp2040_utils import device_detector
from rp2040_utils.device_detector import BoardInfo


_LOG = logging.getLogger()


def parse_args():
    """Parses command-line arguments."""

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        'binary', type=Path, help='The target test binary to run'
    )
    parser.add_argument(
        '--usb-bus',
        type=int,
        help='The bus this Pi Pico is on',
    )
    parser.add_argument(
        '--usb-port',
        type=int,
        help='The port of this Pi Pico on the specified USB bus',
    )
    parser.add_argument(
        '--serial-port',
        type=str,
        help='The name of the serial port to connect to when running tests',
    )
    parser.add_argument(
        '-b',
        '--baud',
        type=int,
        default=115200,
        help='Baud rate to use for serial communication with target device',
    )
    parser.add_argument(
        '--test-timeout',
        type=float,
        default=5.0,
        help='Maximum communication delay in seconds before a '
        'test is considered unresponsive and aborted',
    )
    parser.add_argument(
        '--verbose',
        '-v',
        dest='verbose',
        action='store_true',
        help='Output additional logs as the script runs',
    )

    return parser.parse_args()


class PiPicoTestingDevice(SerialTestingDevice):
    """A SerialTestingDevice implementation for the Pi Pico."""

    def __init__(self, board_info: BoardInfo, baud_rate=115200):
        self._board_info = board_info
        self._baud_rate = baud_rate

    def load_binary(self, binary: Path) -> bool:
        """Flash a binary to this device, returning success or failure."""
        cmd = (
            'picotool',
            'load',
            '-x',
            str(binary),
            '--bus',
            str(self._board_info.bus),
            '--address',
            str(self._board_info.address()),
            '-F',
        )
        _LOG.debug('Flashing ==> %s', ' '.join(cmd))
        process = subprocess.run(
            cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT
        )
        if process.returncode:
            err = (
                'Flashing command failed: ' + ' '.join(cmd),
                str(self._board_info),
                process.stdout.decode('utf-8', errors='replace'),
            )
            _LOG.error('\n\n'.join(err))
            return False

        start_time = time.monotonic()
        load_binary_timeout = 10.0
        # Wait for serial port to enumerate. This will retry forever.
        while time.monotonic() - start_time < load_binary_timeout:
            # If the serial port path isn't known, watch for a newly
            # enmerated path.
            if not self.serial_port() or self.serial_port() == 'None':
                # Wait a bit before checking for a new port.
                time.sleep(0.3)
                # Check for updated serial port path.
                for device in device_detector.detect_boards():
                    if (
                        device.bus == self._board_info.bus
                        and device.port == self._board_info.port
                    ):
                        self._board_info.serial_port = device.serial_port
                        # Serial port found, break out of device for loop.
                        break

            # Serial port known try to connect to it.
            if self.serial_port():
                # Connect to the serial port.
                try:
                    serial.Serial(
                        baudrate=self.baud_rate(), port=self.serial_port()
                    )
                    return True
                except serial.serialutil.SerialException:
                    # Unable to connect, try again.
                    _LOG.debug(
                        'Unable to connect to %s, retrying', self.serial_port()
                    )
                    time.sleep(0.1)

        _LOG.error(
            'Binary flashed but unable to connect to the serial port: %s',
            self.serial_port(),
        )
        return False

    def serial_port(self) -> str:
        if not self._board_info.serial_port:
            return 'None'
        return self._board_info.serial_port

    def baud_rate(self) -> int:
        return self._baud_rate


def _run_test(
    device: PiPicoTestingDevice, binary: Path, test_timeout: float
) -> bool:
    return serial_test_runner.run_device_test(device, binary, test_timeout)


def run_device_test(
    binary: Path,
    test_timeout: float,
    serial_port: str,
    baud_rate: int,
    usb_bus: int,
    usb_port: int,
) -> bool:
    """Flashes, runs, and checks an on-device test binary.

    Returns true on test pass.
    """
    board = BoardInfo(
        bus=usb_bus,
        port=usb_port,
        serial_port=serial_port,
    )
    return _run_test(
        PiPicoTestingDevice(board, baud_rate), binary, test_timeout
    )


def detect_and_run_test(binary: Path, test_timeout: float, baud_rate: int):
    _LOG.debug('Attempting to automatically detect dev board')
    boards = device_detector.detect_boards()
    if not boards:
        error = 'Could not find an attached device'
        _LOG.error(error)
        raise DeviceNotFound(error)
    return _run_test(
        PiPicoTestingDevice(boards[0], baud_rate), binary, test_timeout
    )


def main():
    """Set up runner, and then flash/run device test."""
    args = parse_args()
    log_level = logging.DEBUG if args.verbose else logging.INFO
    pw_cli.log.install(level=log_level)

    test_logfile = args.binary.with_suffix(args.binary.suffix + '.test_log.txt')
    # Truncate existing logfile
    test_logfile.write_text('', encoding='utf-8')
    # Setup the test_log.txt file handler.
    pw_cli.log.install(
        level=logging.DEBUG,
        use_color=False,
        log_file=test_logfile,
        logger=_LOG,
    )

    test_passed = False
    if not args.serial_port:
        test_passed = detect_and_run_test(
            args.binary, args.test_timeout, args.baud
        )
    else:
        test_passed = run_device_test(
            args.binary,
            args.test_timeout,
            args.serial_port,
            args.baud,
            args.usb_bus,
            args.usb_port,
        )

        sys.exit(0 if test_passed else 1)


if __name__ == '__main__':
    main()
