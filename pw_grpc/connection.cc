// Copyright 2024 The Pigweed Authors
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

#include "pw_grpc/connection.h"

#include <cinttypes>
#include <cstring>
#include <string_view>
#include <type_traits>

#include "pw_assert/check.h"
#include "pw_chrono/system_clock.h"
#include "pw_grpc_private/hpack.h"
#include "pw_log/log.h"
#include "pw_preprocessor/compiler.h"
#include "pw_status/try.h"

namespace pw::grpc {
namespace internal {

// RFC 9113 §6
// Enum names left in naming style of RFC
enum class FrameType : uint8_t {
  DATA = 0x00,
  HEADERS = 0x01,
  PRIORITY = 0x02,
  RST_STREAM = 0x03,
  SETTINGS = 0x04,
  PUSH_PROMISE = 0x05,
  PING = 0x06,
  GOAWAY = 0x07,
  WINDOW_UPDATE = 0x08,
  CONTINUATION = 0x09,
};

// RFC 9113 §4.1
constexpr size_t kFrameHeaderEncodedSize = 9;
struct FrameHeader {
  uint32_t payload_length;
  FrameType type;
  uint8_t flags;
  StreamId stream_id;
};

// RFC 9113 §7
// Enum names left in naming style of RFC
enum class Http2Error : uint32_t {
  NO_ERROR = 0x00,
  PROTOCOL_ERROR = 0x01,
  INTERNAL_ERROR = 0x02,
  FLOW_CONTROL_ERROR = 0x03,
  SETTINGS_TIMEOUT = 0x04,
  STREAM_CLOSED = 0x05,
  FRAME_SIZE_ERROR = 0x06,
  REFUSED_STREAM = 0x07,
  CANCEL = 0x08,
  COMPRESSION_ERROR = 0x09,
  CONNECT_ERROR = 0x0a,
  ENHANCE_YOUR_CALM = 0x0b,
  INADEQUATE_SECURITY = 0x0c,
  HTTP_1_1_REQUIRED = 0x0d,
};

}  // namespace internal

namespace {

using internal::FrameHeader;
using internal::FrameType;
using internal::Http2Error;
using internal::kMaxConcurrentStreams;
using internal::kMaxGrpcMessageSize;

// RFC 9113 §3.4
constexpr std::string_view kExpectedConnectionPrefaceLiteral(
    "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n");

static_assert(kMaxMethodNameSize == kHpackMaxStringSize);

enum {
  FLAGS_ACK = 0x01,
  FLAGS_END_STREAM = 0x01,
  FLAGS_END_HEADERS = 0x04,
  FLAGS_PADDED = 0x08,
  FLAGS_PRIORITY = 0x20,
};

// RFC 9113 §6.5.2
enum SettingType : uint16_t {
  SETTINGS_HEADER_TABLE_SIZE = 0x01,
  SETTINGS_ENABLE_PUSH = 0x02,
  SETTINGS_MAX_CONCURRENT_STREAMS = 0x03,
  SETTINGS_INITIAL_WINDOW_SIZE = 0x04,
  SETTINGS_MAX_FRAME_SIZE = 0x05,
  SETTINGS_MAX_HEADER_LIST_SIZE = 0x06,
};

Status ReadExactly(stream::Reader& reader, ByteSpan buffer) {
  size_t bytes_read = 0;
  while (bytes_read < buffer.size()) {
    PW_TRY_ASSIGN(auto out, reader.Read(buffer.subspan(bytes_read)));
    bytes_read += out.size();
  }
  return OkStatus();
}

Result<FrameHeader> ReadFrameHeader(stream::Reader& reader) {
  std::array<std::byte, internal::kFrameHeaderEncodedSize> buffer;
  PW_TRY(ReadExactly(reader, buffer));

  // RFC 9113 §4.1
  FrameHeader out;
  ByteBuilder builder(as_writable_bytes(span{buffer}));
  auto it = builder.begin();
  auto type_and_length = it.ReadUint32(endian::big);
  out.payload_length = type_and_length >> 8;
  out.type = static_cast<FrameType>(type_and_length & 0xff);
  out.flags = it.ReadUint8();
  out.stream_id = it.ReadUint32(endian::big) & 0x7fffffff;
  return out;
}

template <typename T, std::enable_if_t<std::is_integral_v<T>, bool> = true>
constexpr T ToNetworkOrder(T value) {
  return bytes::ConvertOrder(/*from=*/endian::native,
                             /*to=*/endian::big,
                             value);
}

template <typename T, std::enable_if_t<std::is_enum_v<T>, bool> = true>
constexpr std::underlying_type_t<T> ToNetworkOrder(T value) {
  return ToNetworkOrder(static_cast<std::underlying_type_t<T>>(value));
}

// Use this instead of FrameHeader when writing frames.
PW_PACKED(struct) WireFrameHeader {
  WireFrameHeader(FrameHeader h)
      : payload_length_and_type(ToNetworkOrder(h.payload_length << 8 |
                                               static_cast<uint32_t>(h.type))),
        flags(h.flags),
        stream_id(ToNetworkOrder(h.stream_id)) {}

  uint32_t payload_length_and_type;
  uint8_t flags;
  uint32_t stream_id;
};

template <typename T>
ConstByteSpan AsBytes(T& object) {
  return as_bytes(span<T, 1>{&object, 1});
}

// RFC 9113 §6.1
Status SendData(SendQueue& send_queue,
                StreamId stream_id,
                ConstByteSpan payload1,
                ConstByteSpan payload2) {
  PW_LOG_DEBUG("Conn.Send DATA with id=%" PRIu32 " len1=%" PRIu32
               " len2=%" PRIu32,
               stream_id,
               static_cast<uint32_t>(payload1.size()),
               static_cast<uint32_t>(payload2.size()));
  WireFrameHeader frame(FrameHeader{
      .payload_length =
          static_cast<uint32_t>(payload1.size() + payload2.size()),
      .type = FrameType::DATA,
      .flags = 0,
      .stream_id = stream_id,
  });
  std::array<ConstByteSpan, 3> data_vector = {AsBytes(frame)};
  size_t i = 1;
  if (!payload1.empty()) {
    data_vector[i++] = payload1;
  }
  if (!payload2.empty()) {
    data_vector[i++] = payload2;
  }
  PW_TRY(send_queue.SendBytesVector(span{data_vector.data(), i}));
  return OkStatus();
}

// RFC 9113 §6.2
Status SendHeaders(SendQueue& send_queue,
                   StreamId stream_id,
                   ConstByteSpan payload1,
                   ConstByteSpan payload2,
                   bool end_stream) {
  PW_LOG_DEBUG("Conn.Send HEADERS with id=%" PRIu32 " len1=%" PRIu32
               " len2=%" PRIu32 " end=%d",
               stream_id,
               static_cast<uint32_t>(payload1.size()),
               static_cast<uint32_t>(payload2.size()),
               end_stream);
  WireFrameHeader frame(FrameHeader{
      .payload_length =
          static_cast<uint32_t>(payload1.size() + payload2.size()),
      .type = FrameType::HEADERS,
      .flags = FLAGS_END_HEADERS,
      .stream_id = stream_id,
  });

  if (end_stream) {
    frame.flags |= FLAGS_END_STREAM;
  }

  std::array<ConstByteSpan, 3> headers_vector = {AsBytes(frame)};
  size_t i = 1;
  if (!payload1.empty()) {
    headers_vector[i++] = payload1;
  }
  if (!payload2.empty()) {
    headers_vector[i++] = payload2;
  }
  PW_TRY(send_queue.SendBytesVector(span{headers_vector.data(), i}));

  return OkStatus();
}

// RFC 9113 §6.4
Status SendRstStream(SendQueue& send_queue,
                     StreamId stream_id,
                     Http2Error code) {
  PW_PACKED(struct) RstStreamFrame {
    WireFrameHeader header;
    uint32_t error_code;
  };
  RstStreamFrame frame{
      .header = WireFrameHeader(FrameHeader{
          .payload_length = 4,
          .type = FrameType::RST_STREAM,
          .flags = 0,
          .stream_id = stream_id,
      }),
      .error_code = ToNetworkOrder(code),
  };
  PW_TRY(send_queue.SendBytes(AsBytes(frame)));

  return OkStatus();
}

// RFC 9113 §6.9
Status SendWindowUpdates(SendQueue& send_queue,
                         StreamId stream_id,
                         uint32_t increment) {
  // It is illegal to send updates with increment=0.
  if (increment == 0) {
    return OkStatus();
  }
  if (increment & 0x80000000) {
    // Upper bit is reserved, error.
    return Status::InvalidArgument();
  }

  PW_LOG_DEBUG("Conn.Send WINDOW_UPDATE frames with id=%" PRIu32
               " increment=%" PRIu32,
               stream_id,
               increment);

  PW_PACKED(struct) WindowUpdateFrame {
    WireFrameHeader header;
    uint32_t increment;
  };
  WindowUpdateFrame frames[2] = {
      {
          .header = WireFrameHeader(FrameHeader{
              .payload_length = 4,
              .type = FrameType::WINDOW_UPDATE,
              .flags = 0,
              .stream_id = 0,
          }),
          .increment = ToNetworkOrder(increment),
      },
      {
          .header = WireFrameHeader(FrameHeader{
              .payload_length = 4,
              .type = FrameType::WINDOW_UPDATE,
              .flags = 0,
              .stream_id = stream_id,
          }),
          .increment = ToNetworkOrder(increment),
      },
  };
  PW_TRY(send_queue.SendBytes(as_bytes(span{frames})));
  return OkStatus();
}

// RFC 9113 §6.5
Status SendSettingsAck(SendQueue& send_queue) {
  PW_LOG_DEBUG("Conn.Send SETTINGS ACK");
  WireFrameHeader frame(FrameHeader{
      .payload_length = 0,
      .type = FrameType::SETTINGS,
      .flags = FLAGS_ACK,
      .stream_id = 0,
  });
  PW_TRY(send_queue.SendBytes(AsBytes(frame)));
  return OkStatus();
}

}  // namespace

Connection::Connection(stream::ReaderWriter& socket,
                       SendQueue& send_queue,
                       RequestCallbacks& callbacks)
    : socket_(socket),
      send_queue_(send_queue),
      reader_(*this, callbacks),
      writer_(*this) {}

Status Connection::Reader::ProcessFrame() {
  if (!received_connection_preface_) {
    return Status::FailedPrecondition();
  }

  PW_TRY_ASSIGN(auto frame, ReadFrameHeader(connection_.socket_.as_reader()));
  switch (frame.type) {
    // Frames that we handle.
    case FrameType::DATA:
      PW_TRY(ProcessDataFrame(frame));
      break;
    case FrameType::HEADERS:
      PW_TRY(ProcessHeadersFrame(frame));
      break;
    case FrameType::PRIORITY:
      PW_TRY(ProcessIgnoredFrame(frame));
      break;
    case FrameType::RST_STREAM:
      PW_TRY(ProcessRstStreamFrame(frame));
      break;
    case FrameType::SETTINGS:
      PW_TRY(ProcessSettingsFrame(frame, /*send_ack=*/true));
      break;
    case FrameType::PING:
      PW_TRY(ProcessPingFrame(frame));
      break;
    case FrameType::WINDOW_UPDATE:
      PW_TRY(ProcessWindowUpdateFrame(frame));
      break;

    // Frames that trigger an immediate connection close.
    case FrameType::GOAWAY:
      PW_LOG_ERROR("Client sent GOAWAY");
      // don't bother sending GOAWAY in response
      return Status::Internal();
    case FrameType::PUSH_PROMISE:
      PW_LOG_ERROR("Client sent PUSH_PROMISE");
      SendGoAway(Http2Error::PROTOCOL_ERROR);
      return Status::Internal();
    case FrameType::CONTINUATION:
      PW_LOG_ERROR("Client sent CONTINUATION: unsupported");
      SendGoAway(Http2Error::INTERNAL_ERROR);
      return Status::Internal();
  }

  return OkStatus();
}

pw::Result<std::reference_wrapper<Connection::Stream>>
Connection::SharedState::LookupStream(StreamId id) {
  for (size_t i = 0; i < streams.size(); i++) {
    if (streams[i].id == id) {
      return streams[i];
    }
  }
  return Status::NotFound();
}

Status Connection::Writer::SendResponseMessage(StreamId stream_id,
                                               ConstByteSpan message) {
  auto state = connection_.LockState();
  auto stream = state->LookupStream(stream_id);
  if (!stream.ok()) {
    return Status::NotFound();
  }

  if (message.size() > kMaxGrpcMessageSize) {
    PW_LOG_WARN("Message %" PRIu32 " bytes on id=%" PRIu32
                " exceeds maximum message size",
                static_cast<uint32_t>(message.size()),
                stream_id);
    return Status::InvalidArgument();
  }

  // This should block until there is enough send window.
  if (static_cast<int32_t>(message.size()) > stream->get().send_window ||
      static_cast<int32_t>(message.size()) > state->connection_send_window) {
    PW_LOG_WARN("Not enough window to send %" PRIu32 " bytes on id=%" PRIu32,
                static_cast<uint32_t>(message.size()),
                stream_id);
    return Status::ResourceExhausted();
  }

  auto status = OkStatus();
  if (!stream->get().started_response) {
    stream->get().started_response = true;
    status = SendHeaders(connection_.send_queue_,
                         stream_id,
                         ResponseHeadersPayload(),
                         ConstByteSpan(),
                         /*end_stream=*/false);
  }
  if (status.ok()) {
    // Write a Length-Prefixed-Message payload.
    ByteBuffer<5> prefix;
    prefix.PutUint8(0);
    prefix.PutUint32(message.size(), endian::big);
    status = SendData(connection_.send_queue_, stream_id, prefix, message);
  }
  if (!status.ok()) {
    PW_LOG_WARN("Failed sending response message on id=%" PRIu32 " error=%d",
                stream_id,
                status.code());
    return Status::Unavailable();
  }
  stream->get().send_window -= message.size();
  state->connection_send_window -= message.size();
  return OkStatus();
}

Status Connection::Writer::SendResponseComplete(StreamId stream_id,
                                                Status response_code) {
  auto state = connection_.LockState();
  auto stream = state->LookupStream(stream_id);
  if (!stream.ok()) {
    return Status::NotFound();
  }

  Status status;
  if (!stream->get().started_response) {
    // If the response has not started yet, we need to include the initial
    // headers.
    PW_LOG_DEBUG("Conn.SendResponseWithTrailers id=%" PRIu32 " code=%d",
                 stream_id,
                 response_code.code());
    status = SendHeaders(connection_.send_queue_,
                         stream_id,
                         ResponseHeadersPayload(),
                         ResponseTrailersPayload(response_code),
                         /*end_stream=*/true);
  } else {
    PW_LOG_DEBUG("Conn.SendTrailers id=%" PRIu32 " code=%d",
                 stream_id,
                 response_code.code());
    status = SendHeaders(connection_.send_queue_,
                         stream_id,
                         ConstByteSpan(),
                         ResponseTrailersPayload(response_code),
                         /*end_stream=*/true);
  }

  if (!status.ok()) {
    PW_LOG_WARN("Failed sending response complete on id=%" PRIu32 " error=%d",
                stream_id,
                status.code());
    return Status::Unavailable();
  }

  PW_LOG_DEBUG("Conn.CloseStream id=%" PRIu32, stream_id);
  stream->get().Reset();

  return OkStatus();
}

pw::Status Connection::Reader::CreateStream(StreamId id) {
  auto state = connection_.LockState();
  for (size_t i = 0; i < state->streams.size(); i++) {
    if (state->streams[i].id != 0) {
      continue;
    }
    PW_LOG_DEBUG("Conn.CreateStream id=%" PRIu32 " at slot=%" PRIu32,
                 id,
                 static_cast<uint32_t>(i));
    state->streams[i].id = id;
    state->streams[i].half_closed = false;
    state->streams[i].started_response = false;
    state->streams[i].send_window = initial_send_window_;
    return OkStatus();
  }
  PW_LOG_WARN("Conn.CreateStream id=%" PRIu32 " OUT OF SPACE", id);
  return Status::ResourceExhausted();
}

void Connection::Reader::CloseStream(Connection::Stream& stream) {
  StreamId id = stream.id;
  PW_LOG_DEBUG("Conn.CloseStream id=%" PRIu32, id);
  stream.Reset();
  callbacks_.OnCancel(id);
}

// RFC 9113 §3.4
Status Connection::Reader::ProcessConnectionPreface() {
  if (received_connection_preface_) {
    return OkStatus();
  }

  callbacks_.OnNewConnection();

  // The preface starts with a literal string.
  auto literal = span{payload_scratch_}.subspan(
      0, kExpectedConnectionPrefaceLiteral.size());

  PW_TRY(ReadExactly(connection_.socket_.as_reader(), literal));
  if (std::memcmp(literal.data(),
                  kExpectedConnectionPrefaceLiteral.data(),
                  kExpectedConnectionPrefaceLiteral.size()) != 0) {
    PW_LOG_ERROR("Invalid connection preface literal");
    return Status::Internal();
  }

  PW_LOG_DEBUG("Conn.Preface received literal");

  // Client must send a SETTINGS frames.
  PW_TRY_ASSIGN(auto client_frame,
                ReadFrameHeader(connection_.socket_.as_reader()));
  if (client_frame.type != FrameType::SETTINGS) {
    PW_LOG_ERROR(
        "Connection preface missing SETTINGS frame, found frame.type=%d",
        static_cast<int>(client_frame.type));
    return Status::Internal();
  }

  // Don't send an ACK yet, we'll do that below.
  PW_TRY(ProcessSettingsFrame(client_frame, /*send_ack=*/false));
  PW_LOG_DEBUG("Conn.Preface received SETTINGS");

  // We must send a SETTINGS frame.
  // RFC 9113 §6.5.2
  PW_PACKED(struct) Setting {
    uint16_t id;
    uint32_t value;
  };
  PW_PACKED(struct) SettingsFrame {
    WireFrameHeader header;
    Setting settings[2];
  };
  SettingsFrame server_frame{
      .header = WireFrameHeader(FrameHeader{
          .payload_length = 12,
          .type = FrameType::SETTINGS,
          .flags = 0,
          .stream_id = 0,
      }),
      .settings =
          {
              {
                  .id = ToNetworkOrder(SETTINGS_HEADER_TABLE_SIZE),
                  .value = ToNetworkOrder(kHpackDynamicHeaderTableSize),
              },
              {
                  .id = ToNetworkOrder(SETTINGS_MAX_CONCURRENT_STREAMS),
                  .value = ToNetworkOrder(kMaxConcurrentStreams),
              },
          },
  };
  PW_LOG_DEBUG("Conn.Send SETTINGS");
  PW_TRY(connection_.send_queue_.SendBytes(AsBytes(server_frame)));

  // We must ack the client's SETTINGS frame *after* sending our SETTINGS.
  PW_TRY(SendSettingsAck(connection_.send_queue_));

  received_connection_preface_ = true;
  PW_LOG_DEBUG("Conn.Preface complete");
  return OkStatus();
}

// RFC 9113 §6.1
Status Connection::Reader::ProcessDataFrame(const FrameHeader& frame) {
  PW_LOG_DEBUG("Conn.Recv DATA id=%" PRIu32 " flags=0x%x len=%" PRIu32,
               frame.stream_id,
               frame.flags,
               frame.payload_length);

  if (frame.stream_id == 0) {
    // RFC 9113 §6.1: "If a DATA frame is received whose Stream Identifier field
    // is 0x00, the recipient MUST respond with a connection error of type
    // PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }

  // From RFC 9113 §6.9: "A receiver that receives a flow-controlled frame MUST
  // always account for its contribution against the connection flow-control
  // window, unless the receiver treats this as a connection error. This is
  // necessary even if the frame is in error. The sender counts the frame toward
  // the flow-control window, but if the receiver does not, the flow-control
  // window at the sender and receiver can become different."
  //
  // To simplify this, we send WINDOW_UPDATE frames eagerly.
  //
  // In the future we should do something less chatty.
  PW_TRY(SendWindowUpdates(
      connection_.send_queue_, frame.stream_id, frame.payload_length));

  {
    auto state = connection_.LockState();
    auto stream = state->LookupStream(frame.stream_id);
    if (!stream.ok()) {
      PW_LOG_DEBUG("Ignoring DATA on closed stream id=%" PRIu32,
                   frame.stream_id);
      // Stream has been fully closed: silently ignore.
      return OkStatus();
    }

    if (stream->get().half_closed) {
      PW_LOG_ERROR("Recv DATA on half-closed stream id=%" PRIu32,
                   frame.stream_id);
      // RFC 9113 §6.1: "If a DATA frame is received whose stream is not in the
      // "open" or "half-closed (local)" state, the recipient MUST respond with
      // a stream error of type STREAM_CLOSED."
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::STREAM_CLOSED));
      return OkStatus();
    }
  }

  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));

  // Drop padding.
  if ((frame.flags & FLAGS_PADDED) != 0) {
    uint32_t pad_length = static_cast<uint32_t>(payload[0]);
    if (pad_length >= frame.payload_length) {
      // RFC 9113 §6.1: "If the length of the padding is the length of the frame
      // payload or greater, the recipient MUST treat this as a connection error
      // of type PROTOCOL_ERROR."
      SendGoAway(Http2Error::PROTOCOL_ERROR);
      return Status::Internal();
    }
    payload = payload.subspan(1, payload.size() - pad_length - 1);
  }

  // For now this assumes that each DATA frame contains whole messages
  // only. Supporting partial messages is possible but requires buffering the
  // partial messages, which in the worst case requires ~16kB per stream. The
  // best way to do this buffering is TBD.

  while (!payload.empty()) {
    // Parse the grpc Length-Prefixed-Message.
    // https://github.com/grpc/grpc/blob/v1.60.x/doc/PROTOCOL-HTTP2.md#requests
    if (payload.size() < 5) {
      SendGoAway(Http2Error::PROTOCOL_ERROR);
      return Status::Internal();
    }

    ByteBuilder builder(payload);
    auto it = builder.begin();
    auto message_compressed = it.ReadUint8();
    auto message_length = it.ReadUint32(endian::big);
    if (message_compressed != 0) {
      PW_LOG_ERROR("Unsupported: grpc message is compressed");
      auto state = connection_.LockState();
      auto stream = state->LookupStream(frame.stream_id);
      if (!stream.ok()) {
        return OkStatus();
      }
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::INTERNAL_ERROR));
      return OkStatus();
    }
    payload = payload.subspan(5);
    if (message_length > payload.size()) {
      PW_LOG_ERROR("Unsupported: DATA frame contains a partial grpc message");
      auto state = connection_.LockState();
      auto stream = state->LookupStream(frame.stream_id);
      if (!stream.ok()) {
        return OkStatus();
      }
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::PROTOCOL_ERROR));
      return OkStatus();
    }

    if (const auto status = callbacks_.OnMessage(
            frame.stream_id, payload.subspan(0, message_length));
        !status.ok()) {
      auto state = connection_.LockState();
      auto stream = state->LookupStream(frame.stream_id);
      if (!stream.ok()) {
        return OkStatus();
      }
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::INTERNAL_ERROR));
      return OkStatus();
    }
    payload = payload.subspan(message_length);
  }

  // grpc requires every request stream to end with an empty DATA frame with
  // FLAGS_END_STREAM. If a client sends FLAGS_END_STREAM with a non-empty
  // payload, it's not specified how the server should respond. We choose to
  // accept the payload before ending the stream.
  // See: https://github.com/grpc/grpc/blob/v1.60.x/doc/PROTOCOL-HTTP2.md.
  if ((frame.flags & FLAGS_END_STREAM) != 0) {
    {
      auto state = connection_.LockState();
      if (auto stream = state->LookupStream(frame.stream_id); stream.ok()) {
        stream->get().half_closed = true;
      }
    }
    callbacks_.OnHalfClose(frame.stream_id);
  }

  return OkStatus();
}

// RFC 9113 §6.2
Status Connection::Reader::ProcessHeadersFrame(const FrameHeader& frame) {
  PW_LOG_DEBUG("Conn.Recv HEADERS id=%" PRIu32 " len=%" PRIu32,
               frame.stream_id,
               frame.payload_length);

  if (frame.stream_id == 0) {
    // RFC 9113 §6.2: "If a HEADERS frame is received whose Stream Identifier
    // field is 0x00, the recipient MUST respond with a connection error of type
    // PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }
  if (frame.stream_id % 2 != 1 || frame.stream_id <= last_stream_id_) {
    // RFC 9113 §5.1.1: "Streams initiated by a client MUST use odd-numbered
    // stream identifiers ... The identifier of a newly established stream MUST
    // be numerically greater than all streams that the initiating endpoint has
    // opened ... An endpoint that receives an unexpected stream identifier MUST
    // respond with a connection error of type PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }

  last_stream_id_ = frame.stream_id;

  {
    auto state = connection_.LockState();
    if (auto stream = state->LookupStream(frame.stream_id); stream.ok()) {
      PW_LOG_DEBUG("Client sent HEADERS after the first stream message");
      // grpc requests cannot contain trailers.
      // See: https://github.com/grpc/grpc/blob/v1.60.x/doc/PROTOCOL-HTTP2.md.
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::PROTOCOL_ERROR));
      return OkStatus();
    }
  }

  if ((frame.flags & FLAGS_END_STREAM) != 0) {
    PW_LOG_DEBUG("Client sent HEADERS with END_STREAM");
    // grpc requests must send END_STREAM in an empty DATA frame.
    // See: https://github.com/grpc/grpc/blob/v1.60.x/doc/PROTOCOL-HTTP2.md.
    PW_TRY(SendRstStream(
        connection_.send_queue_, frame.stream_id, Http2Error::PROTOCOL_ERROR));
    return OkStatus();
  }
  if ((frame.flags & FLAGS_END_HEADERS) == 0) {
    PW_LOG_ERROR("Client sent HEADERS frame without END_HEADERS: unsupported");
    SendGoAway(Http2Error::INTERNAL_ERROR);
    return Status::Internal();
  }

  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));

  // Drop padding.
  if ((frame.flags & FLAGS_PADDED) != 0) {
    uint32_t pad_length = static_cast<uint32_t>(payload[0]);
    if (pad_length >= frame.payload_length) {
      // RFC 9113 §6.2: "If the length of the padding is the length of the frame
      // payload or greater, the recipient MUST treat this as a connection error
      // of type PROTOCOL_ERROR."
      SendGoAway(Http2Error::PROTOCOL_ERROR);
      return Status::Internal();
    }
    payload = payload.subspan(1, payload.size() - pad_length - 1);
  }

  // Drop priority fields.
  if ((frame.flags & FLAGS_PRIORITY) != 0) {
    payload = payload.subspan(5);
  }

  PW_TRY_ASSIGN(auto method_name, HpackParseRequestHeaders(payload));
  if (!CreateStream(frame.stream_id).ok()) {
    PW_LOG_WARN("Too many streams, rejecting id=%" PRIu32, frame.stream_id);
    return SendRstStream(
        connection_.send_queue_, frame.stream_id, Http2Error::REFUSED_STREAM);
  }

  if (const auto status = callbacks_.OnNew(frame.stream_id, method_name);
      !status.ok()) {
    auto state = connection_.LockState();
    if (auto stream = state->LookupStream(frame.stream_id); stream.ok()) {
      return SendRstStreamAndClose(stream->get(), Http2Error::INTERNAL_ERROR);
    }
  }

  return OkStatus();
}

// RFC 9113 §6.4
Status Connection::Reader::ProcessRstStreamFrame(const FrameHeader& frame) {
  PW_LOG_DEBUG("Conn.Recv RST_STREAM id=%" PRIu32 " len=%" PRIu32,
               frame.stream_id,
               frame.payload_length);

  if (frame.stream_id == 0) {
    // RFC 9113 §6.4: "If a RST_STREAM frame is received with a stream
    // identifier of 0x00, the recipient MUST treat this as a connection error
    // of type PROTOCOL_ERROR".
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }
  if (frame.stream_id > last_stream_id_) {
    // RFC 9113 §6.4: "If a RST_STREAM frame identifying an idle stream is
    // received, the recipient MUST treat this as a connection error of type
    // PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }
  if (frame.payload_length != 4) {
    // RFC 9113 §6.4: "A RST_STREAM frame with a length other than 4 octets MUST
    // be treated as a connection error of type FRAME_SIZE_ERROR."
    SendGoAway(Http2Error::FRAME_SIZE_ERROR);
    return Status::Internal();
  }

  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));
  ByteBuilder builder(payload);
  auto error_code = builder.begin().ReadUint32(endian::big);

  PW_LOG_DEBUG("Conn.RstStream id=%" PRIu32 " error=%" PRIu32,
               frame.stream_id,
               error_code);
  auto state = connection_.LockState();
  if (auto stream = state->LookupStream(frame.stream_id); stream.ok()) {
    CloseStream(stream->get());
  }
  return OkStatus();
}

// RFC 9113 §6.5
Status Connection::Reader::ProcessSettingsFrame(const FrameHeader& frame,
                                                bool send_ack) {
  PW_LOG_DEBUG("Conn.Recv SETTINGS len=%" PRIu32 " flags=0x%x",
               frame.payload_length,
               frame.flags);

  if ((frame.flags & FLAGS_ACK) != 0) {
    // RFC 9113 §6.5: "Receipt of a SETTINGS frame with the ACK flag set and a
    // length field value other than 0 MUST be treated as a connection error of
    // type FRAME_SIZE_ERROR."
    if (frame.payload_length != 0) {
      PW_LOG_ERROR("Invalid SETTINGS frame: has ACK with non-empty payload");
      SendGoAway(Http2Error::FRAME_SIZE_ERROR);
      return Status::Internal();
    }
    // Don't ACK an ACK.
    send_ack = false;
  } else {
    // RFC 9113 §6.5: "A SETTINGS frame with a length other than a multiple of 6
    // octets MUST be treated as a connection error of type FRAME_SIZE_ERROR."
    if (frame.payload_length % 6 != 0) {
      PW_LOG_ERROR("Invalid SETTINGS frame: payload size invalid");
      SendGoAway(Http2Error::FRAME_SIZE_ERROR);
      return Status::Internal();
    }
  }

  if (frame.stream_id != 0) {
    // RFC 9113 §6.5: "If an endpoint receives a SETTINGS frame whose Stream
    // Identifier field is anything other than 0x00, the endpoint MUST respond
    // with a connection error of type PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }

  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));

  // RFC 9113 §6.5.2
  ByteBuilder builder(payload);
  for (auto it = builder.begin(); it != builder.end();) {
    auto id = it.ReadUint16(endian::big);
    auto value = it.ReadUint32(endian::big);
    PW_LOG_DEBUG("Applying SETTING id=%" PRIu16 " value=%" PRIu32, id, value);
    switch (id) {
      case SETTINGS_INITIAL_WINDOW_SIZE: {
        // RFC 9113 §6.5.2: "Values above the maximum flow-control window size
        // of 2^31-1 MUST be treated as a connection error of type
        // FLOW_CONTROL_ERROR."
        if ((value & (1 << 31)) != 0) {
          SendGoAway(Http2Error::FLOW_CONTROL_ERROR);
          return Status::Internal();
        }
        // RFC 9113 §6.9.2: "When the value of SETTINGS_INITIAL_WINDOW_SIZE
        // changes, a receiver MUST adjust the size of all stream flow-control
        // windows that it maintains by the difference between the new value and
        // the old value."
        int32_t newval = static_cast<int32_t>(value);
        int32_t delta = newval - initial_send_window_;
        auto state = connection_.LockState();
        for (size_t i = 0; i < state->streams.size(); i++) {
          if (state->streams[i].id == 0) {
            continue;
          }
          if (PW_ADD_OVERFLOW(state->streams[i].send_window,
                              delta,
                              &state->streams[i].send_window)) {
            SendGoAway(Http2Error::FLOW_CONTROL_ERROR);
            return Status::Internal();
          }
        }
        initial_send_window_ = newval;
        break;
      }
      case SETTINGS_MAX_FRAME_SIZE:
        // RFC 9113 §6.5.2: "Values outside this range MUST be treated as a
        // connection error of type PROTOCOL_ERROR".
        if (value < 16384 || 16777215 < value) {
          SendGoAway(Http2Error::PROTOCOL_ERROR);
          return Status::Internal();
        }
        // We never send frame payloads larger than 16384, so we don't need to
        // track the client's preference.
        break;
      // Ignore these.
      // SETTINGS_HEADER_TABLE_SIZE: our responses don't use the dynamic table
      // SETTINGS_ENABLE_PUSH: we don't support push
      // SETTINGS_MAX_CONCURRENT_STREAMS: we don't support push
      // SETTINGS_MAX_HEADER_LIST_SIZE: we send very tiny response HEADERS
      default:
        break;
    }
  }

  if (send_ack) {
    PW_TRY(SendSettingsAck(connection_.send_queue_));
  }

  return OkStatus();
}

// RFC 9113 §6.7
Status Connection::Reader::ProcessPingFrame(const FrameHeader& frame) {
  PW_LOG_DEBUG("Conn.Recv PING len=%" PRIu32, frame.payload_length);

  if (frame.stream_id != 0) {
    // RFC 9113 §6.7: "If a PING frame is received with a Stream Identifier
    // field value other than 0x00, the recipient MUST respond with a connection
    // error of type PROTOCOL_ERROR."
    SendGoAway(Http2Error::PROTOCOL_ERROR);
    return Status::Internal();
  }
  if (frame.payload_length != 8) {
    // RFC 9113 §6.7: "Receipt of a PING frame with a length field value other
    // than 8 MUST be treated as a connection error of type FRAME_SIZE_ERROR."
    SendGoAway(Http2Error::FRAME_SIZE_ERROR);
    return Status::Internal();
  }

  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));

  // Don't ACK an ACK.
  if ((frame.flags & FLAGS_ACK) != 0) {
    return OkStatus();
  }

  // Send an ACK.
  PW_PACKED(struct) PingFrame {
    WireFrameHeader header;
    uint64_t opaque_data;
  };
  ByteBuilder builder(payload);
  PingFrame ack_frame = {
      .header = WireFrameHeader(FrameHeader{
          .payload_length = 8,
          .type = FrameType::PING,
          .flags = FLAGS_ACK,
          .stream_id = 0,
      }),
      // Since we're going to echo this, read as native endian so it gets echoed
      // exactly as-is.
      .opaque_data = builder.begin().ReadUint64(endian::native),
  };
  PW_TRY(connection_.send_queue_.SendBytes(AsBytes(ack_frame)));
  return OkStatus();
}

// RFC 9113 §6.9
Status Connection::Reader::ProcessWindowUpdateFrame(const FrameHeader& frame) {
  PW_LOG_DEBUG("Conn.Recv WINDOW_UPDATE id=%" PRIu32 " len=%" PRIu32,
               frame.stream_id,
               frame.payload_length);

  if (frame.payload_length != 4) {
    // RFC 9113 §6.9: "A WINDOW_UPDATE frame with a length other than 4 octets
    // MUST be treated as a connection error of type FRAME_SIZE_ERROR."
    SendGoAway(Http2Error::FRAME_SIZE_ERROR);
    return Status::Internal();
  }

  // Read window size increment.
  PW_TRY_ASSIGN(auto payload, ReadFramePayload(frame));
  ByteBuilder builder(payload);
  int32_t delta = static_cast<int32_t>(builder.begin().ReadUint32(endian::big) &
                                       0x7fffffff);

  auto state = connection_.LockState();
  auto stream = state->LookupStream(frame.stream_id);

  if (delta == 0) {
    // RFC 9113 §6.9: "A receiver MUST treat a WINDOW_UPDATE frame with an
    // increment of 0 as a stream error of type PROTOCOL_ERROR; errors on the
    // connection flow-control window MUST be treated as a connection error."
    if (frame.stream_id == 0) {
      SendGoAway(Http2Error::PROTOCOL_ERROR);
      return Status::Internal();
    } else {
      if (!stream.ok()) {
        // Already closed
        return OkStatus();
      }
      PW_TRY(SendRstStreamAndClose(stream->get(), Http2Error::PROTOCOL_ERROR));
      return OkStatus();
    }
  }

  // RFC 9113 §6.9.1: "If a sender receives a WINDOW_UPDATE that causes a
  // flow-control window to exceed 2^31-1 bytes, it MUST terminate either the
  // stream or the connection, as appropriate ... with an error code of
  // FLOW_CONTROL_ERROR"
  if (frame.stream_id == 0) {
    if (PW_ADD_OVERFLOW(state->connection_send_window,
                        delta,
                        &state->connection_send_window)) {
      SendGoAway(Http2Error::FLOW_CONTROL_ERROR);
      return Status::Internal();
    }
  } else if (stream.ok()) {
    if (PW_ADD_OVERFLOW(
            stream->get().send_window, delta, &stream->get().send_window)) {
      PW_TRY(
          SendRstStreamAndClose(stream->get(), Http2Error::FLOW_CONTROL_ERROR));
      return OkStatus();
    }
  }

  return OkStatus();
}

// Advance past the payload.
Status Connection::Reader::ProcessIgnoredFrame(const FrameHeader& frame) {
  PW_TRY(ReadFramePayload(frame));
  return OkStatus();
}

Result<ByteSpan> Connection::Reader::ReadFramePayload(
    const FrameHeader& frame) {
  if (frame.payload_length == 0) {
    return ByteSpan();
  }
  if (frame.payload_length >= payload_scratch_.size()) {
    PW_LOG_ERROR("Frame type=%d payload too large: %" PRIu32 " > %" PRIu32,
                 static_cast<int>(frame.type),
                 frame.payload_length,
                 static_cast<uint32_t>(payload_scratch_.size()));
    SendGoAway(Http2Error::FRAME_SIZE_ERROR);
    return Status::Internal();
  }
  auto payload = span{payload_scratch_}.subspan(0, frame.payload_length);
  PW_TRY(ReadExactly(connection_.socket_.as_reader(), payload));
  return payload;
}

// RFC 9113 §6.8
void Connection::Reader::SendGoAway(Http2Error code) {
  if (!received_connection_preface_) {
    // RFC 9113 §3.4: "A GOAWAY frame MAY be omitted in this case, since an
    // invalid preface indicates that the peer is not using HTTP/2."
    return;
  }

  // Close all open streams.
  {
    auto state = connection_.LockState();
    for (size_t i = 0; i < state->streams.size(); i++) {
      if (state->streams[i].id != 0) {
        CloseStream(state->streams[i]);
      }
    }
  }

  PW_PACKED(struct) GoAwayFrame {
    WireFrameHeader header;
    uint32_t last_stream_id;
    uint32_t error_code;
  };
  GoAwayFrame frame{
      .header = WireFrameHeader(FrameHeader{
          .payload_length = 8,
          .type = FrameType::GOAWAY,
          .flags = 0,
          .stream_id = 0,
      }),
      .last_stream_id = ToNetworkOrder(last_stream_id_),
      .error_code = ToNetworkOrder(code),
  };
  // Ignore errors since we're about to close the connection anyway.
  connection_.send_queue_.SendBytes(AsBytes(frame)).IgnoreError();
}

// RFC 9113 §6.4
Status Connection::Reader::SendRstStreamAndClose(Stream& stream,
                                                 Http2Error code) {
  // Ignore errors as we are closing anyways.
  SendRstStream(connection_.send_queue_, stream.id, code).IgnoreError();
  CloseStream(stream);
  return OkStatus();
}

}  // namespace pw::grpc
