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

// This file defines the ServerReaderWriter, ServerReader, and ServerWriter
// classes for the Nanopb RPC interface. These classes are used for
// bidirectional, client, and server streaming RPCs.
#pragma once

#include "pw_rpc/channel.h"
#include "pw_rpc/internal/client_call.h"
#include "pw_rpc/nanopb/internal/common.h"

namespace pw::rpc {
namespace internal {

// Base class for unary and client streaming calls.
template <typename Response>
class NanopbUnaryResponseClientCall : public UnaryResponseClientCall {
 public:
  template <typename CallType, typename... Request>
  static CallType Start(Endpoint& client,
                        uint32_t channel_id,
                        uint32_t service_id,
                        uint32_t method_id,
                        const NanopbMethodSerde& serde,
                        Function<void(const Response&, Status)> on_completed,
                        Function<void(Status)> on_error,
                        const Request&... request) {
    CallType call(client, channel_id, service_id, method_id, serde);

    call.set_on_completed(std::move(on_completed));
    call.set_on_error(std::move(on_error));

    if constexpr (sizeof...(Request) == 0u) {
      call.SendInitialRequest({});
    } else {
      NanopbSendInitialRequest(call, serde.request(), &request...);
    }
    return call;
  }

 protected:
  constexpr NanopbUnaryResponseClientCall() = default;

  NanopbUnaryResponseClientCall(internal::Endpoint& client,
                                uint32_t channel_id,
                                uint32_t service_id,
                                uint32_t method_id,
                                MethodType type,
                                const NanopbMethodSerde& serde)
      : UnaryResponseClientCall(
            client, channel_id, service_id, method_id, type),
        serde_(&serde) {}

  NanopbUnaryResponseClientCall(NanopbUnaryResponseClientCall&& other) {
    *this = std::move(other);
  }

  NanopbUnaryResponseClientCall& operator=(
      NanopbUnaryResponseClientCall&& other) {
    UnaryResponseClientCall::operator=(std::move(other));
    serde_ = other.serde_;
    set_on_completed(std::move(other.nanopb_on_completed_));
    return *this;
  }

  void set_on_completed(
      Function<void(const Response& response, Status)> on_completed) {
    nanopb_on_completed_ = std::move(on_completed);

    UnaryResponseClientCall::set_on_completed(
        [this](ConstByteSpan payload, Status status) {
          if (nanopb_on_completed_) {
            Response response_struct{};
            if (serde_->DecodeResponse(payload, &response_struct)) {
              nanopb_on_completed_(response_struct, status);
            } else {
              on_error(Status::DataLoss());
            }
          }
        });
  }

  Status SendClientStream(const void* payload) {
    if (!active()) {
      return Status::FailedPrecondition();
    }
    return NanopbSendStream(*this, payload, serde_->request());
  }

 private:
  const NanopbMethodSerde* serde_;
  Function<void(const Response&, Status)> nanopb_on_completed_;
};

// Base class for server and bidirectional streaming calls.
template <typename Response>
class NanopbStreamResponseClientCall : public StreamResponseClientCall {
 public:
  template <typename CallType, typename... Request>
  static CallType Start(Endpoint& client,
                        uint32_t channel_id,
                        uint32_t service_id,
                        uint32_t method_id,
                        const NanopbMethodSerde& serde,
                        Function<void(const Response&)> on_next,
                        Function<void(Status)> on_completed,
                        Function<void(Status)> on_error,
                        const Request&... request) {
    CallType call(client, channel_id, service_id, method_id, serde);

    call.set_on_next(std::move(on_next));
    call.set_on_completed(std::move(on_completed));
    call.set_on_error(std::move(on_error));

    if constexpr (sizeof...(Request) == 0u) {
      call.SendInitialRequest({});
    } else {
      NanopbSendInitialRequest(call, serde.request(), &request...);
    }
    return call;
  }

 protected:
  constexpr NanopbStreamResponseClientCall() = default;

  NanopbStreamResponseClientCall(NanopbStreamResponseClientCall&& other) {
    *this = std::move(other);
  }

  NanopbStreamResponseClientCall& operator=(
      NanopbStreamResponseClientCall&& other) {
    StreamResponseClientCall::operator=(std::move(other));
    serde_ = other.serde_;
    set_on_next(std::move(other.nanopb_on_next_));
    return *this;
  }

  NanopbStreamResponseClientCall(internal::Endpoint& client,
                                 uint32_t channel_id,
                                 uint32_t service_id,
                                 uint32_t method_id,
                                 MethodType type,
                                 const NanopbMethodSerde& serde)
      : StreamResponseClientCall(
            client, channel_id, service_id, method_id, type),
        serde_(&serde) {}

  void set_on_next(Function<void(const Response& response)> on_next) {
    nanopb_on_next_ = std::move(on_next);

    internal::Call::set_on_next([this](ConstByteSpan payload) {
      if (nanopb_on_next_) {
        Response response_struct{};
        if (serde_->DecodeResponse(payload, &response_struct)) {
          nanopb_on_next_(response_struct);
        } else {
          on_error(Status::DataLoss());
        }
      }
    });
  }

  Status SendClientStream(const void* payload) {
    if (!active()) {
      return Status::FailedPrecondition();
    }
    return NanopbSendStream(*this, payload, serde_->request());
  }

 private:
  const NanopbMethodSerde* serde_;
  Function<void(const Response&)> nanopb_on_next_;
};

}  // namespace internal

// The NanopbClientReaderWriter is used to send and receive messages in a
// bidirectional streaming RPC.
template <typename Request, typename ResponseType>
class NanopbClientReaderWriter
    : private internal::NanopbStreamResponseClientCall<ResponseType> {
 public:
  using Response = ResponseType;

  constexpr NanopbClientReaderWriter() = default;

  NanopbClientReaderWriter(NanopbClientReaderWriter&&) = default;
  NanopbClientReaderWriter& operator=(NanopbClientReaderWriter&&) = default;

  using internal::Call::active;
  using internal::Call::channel_id;

  // Writes a response struct. Returns the following Status codes:
  //
  //   OK - the response was successfully sent
  //   FAILED_PRECONDITION - the writer is closed
  //   INTERNAL - pw_rpc was unable to encode the Nanopb protobuf
  //   other errors - the ChannelOutput failed to send the packet; the error
  //       codes are determined by the ChannelOutput implementation
  //
  Status Write(const Request& request) {
    return internal::NanopbStreamResponseClientCall<Response>::SendClientStream(
        &request);
  }

  using internal::Call::Cancel;

  // Functions for setting RPC event callbacks.
  using internal::Call::set_on_error;
  using internal::StreamResponseClientCall::set_on_completed;
  using internal::NanopbStreamResponseClientCall<Response>::set_on_next;

 private:
  friend class internal::NanopbStreamResponseClientCall<Response>;

  NanopbClientReaderWriter(internal::Endpoint& client,
                           uint32_t channel_id_value,
                           uint32_t service_id,
                           uint32_t method_id,
                           const internal::NanopbMethodSerde& serde)
      : internal::NanopbStreamResponseClientCall<Response>(
            client,
            channel_id_value,
            service_id,
            method_id,
            MethodType::kBidirectionalStreaming,
            serde) {}
};

// The NanopbClientReader is used to receive messages and send a response in a
// client streaming RPC.
template <typename ResponseType>
class NanopbClientReader
    : private internal::NanopbStreamResponseClientCall<ResponseType> {
 public:
  using Response = ResponseType;

  constexpr NanopbClientReader() = default;

  NanopbClientReader(NanopbClientReader&&) = default;
  NanopbClientReader& operator=(NanopbClientReader&&) = default;

  using internal::Call::active;
  using internal::Call::channel_id;

  // Functions for setting RPC event callbacks.
  using internal::NanopbStreamResponseClientCall<Response>::set_on_next;
  using internal::Call::set_on_error;
  using internal::StreamResponseClientCall::set_on_completed;

  using internal::Call::Cancel;

 private:
  friend class internal::NanopbStreamResponseClientCall<Response>;

  NanopbClientReader(internal::Endpoint& client,
                     uint32_t channel_id_value,
                     uint32_t service_id,
                     uint32_t method_id,
                     const internal::NanopbMethodSerde& serde)
      : internal::NanopbStreamResponseClientCall<Response>(
            client,
            channel_id_value,
            service_id,
            method_id,
            MethodType::kServerStreaming,
            serde) {}
};

// The NanopbClientWriter is used to send responses in a server streaming RPC.
template <typename Request, typename ResponseType>
class NanopbClientWriter
    : private internal::NanopbUnaryResponseClientCall<ResponseType> {
 public:
  using Response = ResponseType;

  constexpr NanopbClientWriter() = default;

  NanopbClientWriter(NanopbClientWriter&&) = default;
  NanopbClientWriter& operator=(NanopbClientWriter&&) = default;

  using internal::Call::active;
  using internal::Call::channel_id;

  // Functions for setting RPC event callbacks.
  using internal::NanopbUnaryResponseClientCall<Response>::set_on_completed;
  using internal::Call::set_on_error;

  Status Write(const Request& request) {
    return internal::NanopbUnaryResponseClientCall<Response>::SendClientStream(
        &request);
  }

  using internal::Call::Cancel;

 private:
  friend class internal::NanopbUnaryResponseClientCall<Response>;

  NanopbClientWriter(internal::Endpoint& client,
                     uint32_t channel_id_value,
                     uint32_t service_id,
                     uint32_t method_id,
                     const internal::NanopbMethodSerde& serde)
      : internal::NanopbUnaryResponseClientCall<Response>(
            client,
            channel_id_value,
            service_id,
            method_id,
            MethodType::kClientStreaming,
            serde) {}
};

// The NanopbUnaryReceiver is used to receive a response in a unary RPC.
template <typename ResponseType>
class NanopbUnaryReceiver
    : private internal::NanopbUnaryResponseClientCall<ResponseType> {
 public:
  using Response = ResponseType;

  constexpr NanopbUnaryReceiver() = default;

  NanopbUnaryReceiver(NanopbUnaryReceiver&&) = default;
  NanopbUnaryReceiver& operator=(NanopbUnaryReceiver&&) = default;

  using internal::Call::active;
  using internal::Call::channel_id;

  // Functions for setting RPC event callbacks.
  using internal::NanopbUnaryResponseClientCall<Response>::set_on_completed;
  using internal::Call::set_on_error;

  using internal::Call::Cancel;

 private:
  friend class internal::NanopbUnaryResponseClientCall<Response>;

  NanopbUnaryReceiver(internal::Endpoint& client,
                      uint32_t channel_id_value,
                      uint32_t service_id,
                      uint32_t method_id,
                      const internal::NanopbMethodSerde& serde)
      : internal::NanopbUnaryResponseClientCall<Response>(client,
                                                          channel_id_value,
                                                          service_id,
                                                          method_id,
                                                          MethodType::kUnary,
                                                          serde) {}
};

}  // namespace pw::rpc
