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
"""Device classes to interact with targets via RPC."""

import logging
from pathlib import Path
from types import ModuleType
from typing import Any, List, Union, Optional

from pw_hdlc.rpc import HdlcRpcClient, default_channels
import pw_log_tokenized

from pw_log.proto import log_pb2
from pw_rpc import callback_client, console_tools
from pw_status import Status
from pw_tokenizer.detokenize import Detokenizer
from pw_tokenizer.proto import decode_optionally_tokenized

# Internal log for troubleshooting this tool (the console).
_LOG = logging.getLogger('tools')
DEFAULT_DEVICE_LOGGER = logging.getLogger('rpc_device')


class Device:
    """Represents an RPC Client for a device running a Pigweed target.

    The target must have and RPC support, RPC logging.
    Note: use this class as a base for specialized device representations.
    """
    def __init__(self,
                 channel_id: int,
                 read,
                 write,
                 proto_library: List[Union[ModuleType, Path]],
                 detokenizer: Optional[Detokenizer],
                 rpc_timeout_s=5):
        self.channel_id = channel_id
        self.protos = proto_library
        self.detokenizer = detokenizer

        self.logger = DEFAULT_DEVICE_LOGGER
        self.logger.setLevel(logging.DEBUG)  # Allow all device logs through.

        callback_client_impl = callback_client.Impl(
            default_unary_timeout_s=rpc_timeout_s,
            default_stream_timeout_s=None,
        )
        self.client = HdlcRpcClient(
            read,
            self.protos,
            default_channels(write),
            lambda data: self.logger.info("%s", str(data)),
            client_impl=callback_client_impl)

        # Start listening to logs as soon as possible.
        self.listen_to_log_stream()

    def info(self) -> console_tools.ClientInfo:
        return console_tools.ClientInfo('device', self.rpcs,
                                        self.client.client)

    @property
    def rpcs(self) -> Any:
        """Returns an object for accessing services on the specified channel."""
        return next(iter(self.client.client.channels())).rpcs

    def listen_to_log_stream(self):
        """Opens a log RPC for the device's unrequested log stream.

        The RPCs remain open until the server cancels or closes them, either
        with a response or error packet.
        """
        self.rpcs.pw.log.Logs.Listen.open(
            on_next=lambda _, log_entries_proto: self.
            _log_entries_proto_parser(log_entries_proto),
            on_completed=lambda _, status: _LOG.info(
                'Log stream completed with status: %s', status),
            on_error=lambda _, error: self._handle_log_stream_error(error))

    def _handle_log_stream_error(self, error: Status):
        """Resets the log stream RPC on error to avoid losing logs."""
        _LOG.error('Log stream error: %s', error)
        self.listen_to_log_stream()

    def _log_entries_proto_parser(self, log_entries_proto: log_pb2.LogEntries):
        for log_proto in log_entries_proto.entries:
            timestamp = log_proto.timestamp
            level = (log_proto.line_level & 0x7)
            if self.detokenizer:
                message = str(
                    decode_optionally_tokenized(self.detokenizer,
                                                log_proto.message))
            else:
                message = log_proto.message.decode("utf-8")
            log = pw_log_tokenized.FormatStringWithMetadata(message)
            self._emit_device_log(level, '', timestamp, log.module,
                                  log.message, **dict(log.fields))

    def _emit_device_log(self, level: int, source_name: str, timestamp: float,
                         module_name: str, message: str, **metadata_fields):
        fields = metadata_fields
        fields['source_name'] = source_name
        fields['timestamp'] = timestamp
        self.logger.log(level * 10,
                        '[%s] %16.3f %s%s',
                        source_name,
                        timestamp,
                        f'{module_name} '.lstrip(),
                        message,
                        extra=dict(extra_metadata_fields=fields))
