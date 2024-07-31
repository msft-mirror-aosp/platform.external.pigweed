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

#include "pw_channel/stream_channel.h"

#include "pw_async2/dispatcher_base.h"
#include "pw_log/log.h"
#include "pw_multibuf/multibuf.h"
#include "pw_status/status.h"
#include "pw_status/try.h"
#include "pw_thread/detached_thread.h"

namespace pw::channel {

using pw::OkStatus;
using pw::Result;
using pw::Status;
using pw::async2::Context;
using pw::async2::Pending;
using pw::async2::Poll;
using pw::async2::Ready;
using pw::async2::WaitReason;
using pw::async2::Waker;
using pw::channel::ByteReaderWriter;
using pw::multibuf::MultiBuf;
using pw::multibuf::MultiBufAllocationFuture;
using pw::multibuf::MultiBufAllocator;
using pw::multibuf::OwnedChunk;
using pw::sync::InterruptSpinLock;
using pw::sync::ThreadNotification;

namespace internal {

bool StreamChannelReadState::HasBufferToFill() {
  std::lock_guard lock(buffer_lock_);
  return !buffer_to_fill_.empty();
}

void StreamChannelReadState::ProvideBufferToFill(MultiBuf&& buf) {
  {
    std::lock_guard lock(buffer_lock_);
    buffer_to_fill_.PushSuffix(std::move(buf));
  }
  buffer_to_fill_available_.release();
}

Poll<Result<MultiBuf>> StreamChannelReadState::PendFilledBuffer(Context& cx) {
  std::lock_guard lock(buffer_lock_);
  if (!filled_buffer_.empty()) {
    return std::move(filled_buffer_);
  }
  // Return an error status only after pulling all the data.
  if (!status_.ok()) {
    return status_;
  }
  on_buffer_filled_ = cx.GetWaker(pw::async2::WaitReason::Unspecified());
  return Pending();
}

void StreamChannelReadState::ReadLoop(pw::stream::Reader& reader) {
  while (true) {
    OwnedChunk buffer = WaitForBufferToFillAndTakeFrontChunk();
    Result<pw::ByteSpan> read = reader.Read(buffer);
    if (!read.ok()) {
      PW_LOG_ERROR("Failed to read from stream in StreamChannel.");
      SetReadError(read.status());
      return;
    }
    buffer->Truncate(read->size());
    ProvideFilledBuffer(MultiBuf::FromChunk(std::move(buffer)));
  }
}

OwnedChunk StreamChannelReadState::WaitForBufferToFillAndTakeFrontChunk() {
  while (true) {
    {
      std::lock_guard lock(buffer_lock_);
      if (!buffer_to_fill_.empty()) {
        return buffer_to_fill_.TakeFrontChunk();
      }
    }
    buffer_to_fill_available_.acquire();
  }
  PW_UNREACHABLE;
}

void StreamChannelReadState::ProvideFilledBuffer(MultiBuf&& filled_buffer) {
  std::lock_guard lock(buffer_lock_);
  filled_buffer_.PushSuffix(std::move(filled_buffer));
  std::move(on_buffer_filled_).Wake();
}

void StreamChannelReadState::SetReadError(Status status) {
  std::lock_guard lock(buffer_lock_);
  status_ = status;
}

Status StreamChannelWriteState::SendData(MultiBuf&& buf) {
  {
    std::lock_guard lock(buffer_lock_);
    if (!status_.ok()) {
      return status_;
    }
    buffer_to_write_.PushSuffix(std::move(buf));
  }
  data_available_.release();
  return OkStatus();
}

void StreamChannelWriteState::WriteLoop(pw::stream::Writer& writer) {
  while (true) {
    data_available_.acquire();
    MultiBuf buffer;
    {
      std::lock_guard lock(buffer_lock_);
      if (buffer_to_write_.empty()) {
        continue;
      }
      buffer = std::move(buffer_to_write_);
    }
    for (const auto& chunk : buffer.Chunks()) {
      Status status = writer.Write(chunk);
      if (!status.ok()) {
        PW_LOG_ERROR("Failed to write to stream in StreamChannel.");
        std::lock_guard lock(buffer_lock_);
        status_ = status;
        return;
      }
    }
  }
}

}  // namespace internal

static constexpr size_t kMinimumReadSize = 64;
static constexpr size_t kDesiredReadSize = 1024;

StreamChannel::StreamChannel(MultiBufAllocator& allocator,
                             pw::stream::Reader& reader,
                             const pw::thread::Options& read_thread_options,
                             pw::stream::Writer& writer,
                             const pw::thread::Options& write_thread_options)
    : reader_(reader),
      writer_(writer),
      read_state_(),
      write_state_(),
      allocation_future_(std::nullopt),
      allocator_(&allocator),
      write_token_(0) {
  pw::thread::DetachedThread(read_thread_options,
                             [this]() { read_state_.ReadLoop(reader_); });
  pw::thread::DetachedThread(write_thread_options,
                             [this]() { write_state_.WriteLoop(writer_); });
}

Status StreamChannel::ProvideBufferIfAvailable(Context& cx) {
  if (read_state_.HasBufferToFill()) {
    return OkStatus();
  }

  if (!allocation_future_.has_value()) {
    allocation_future_ =
        allocator_->AllocateContiguousAsync(kMinimumReadSize, kDesiredReadSize);
  }
  Poll<std::optional<MultiBuf>> maybe_multibuf = allocation_future_->Pend(cx);

  // If this is pending, we'll be awoken and this function will be re-run
  // when a buffer becomes available, allowing us to provide a buffer.
  if (maybe_multibuf.IsPending()) {
    return OkStatus();
  }

  allocation_future_ = std::nullopt;

  if (!maybe_multibuf->has_value()) {
    PW_LOG_ERROR("Failed to allocate multibuf for reading");
    return Status::ResourceExhausted();
  }

  read_state_.ProvideBufferToFill(std::move(**maybe_multibuf));
  return OkStatus();
}

Poll<Result<MultiBuf>> StreamChannel::DoPendRead(Context& cx) {
  PW_TRY(ProvideBufferIfAvailable(cx));
  return read_state_.PendFilledBuffer(cx);
}

Poll<Status> StreamChannel::DoPendReadyToWrite(Context&) { return OkStatus(); }

pw::Result<pw::channel::WriteToken> StreamChannel::DoWrite(
    pw::multibuf::MultiBuf&& data) {
  write_state_.SendData(std::move(data));
  const uint32_t token = write_token_++;
  return CreateWriteToken(token);
}

}  // namespace pw::channel
