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

#include "pw_allocator/allocator.h"
#include "pw_async2/dispatcher.h"
#include "pw_status/status.h"

namespace {

using ::pw::OkStatus;
using ::pw::Result;
using ::pw::Status;
using ::pw::allocator::Allocator;
using ::pw::async2::Context;
using ::pw::async2::Poll;

class MyData {
 public:
};

class ReceiveFuture {
 public:
  Poll<Result<MyData>> Pend(Context&) { return MyData(); }
};

class MyReceiver {
 public:
  ReceiveFuture Receive() { return ReceiveFuture(); }
};

class SendFuture {
 public:
  Poll<Status> Pend(Context&) { return OkStatus(); }
};

class MySender {
 public:
  SendFuture Send(MyData&&) { return SendFuture(); }
};

}  // namespace

// NOTE: we double-include so that the example shows the `#includes`, but we
// can still define types beforehand.

// DOCSTAG: [pw_async2-examples-coro-injection]
#include "pw_allocator/allocator.h"
#include "pw_async2/coro.h"
#include "pw_result/result.h"

namespace {

using ::pw::OkStatus;
using ::pw::Result;
using ::pw::Status;
using ::pw::allocator::Allocator;
using ::pw::async2::Coro;
using ::pw::async2::CoroContext;

class MyReceiver;
class MySender;

/// Create a coroutine which asynchronously receives a value from
/// ``receiver`` and forwards it to ``sender``.
///
/// Note: the ``Allocator`` argument is used by the ``Coro<T>`` internals to
/// allocate the coroutine state. If this allocation fails, ``Coro<Status>``
/// will return ``Status::Internal()``.
Coro<Status> ReceiveAndSend(CoroContext&,
                            MyReceiver receiver,
                            MySender sender) {
  pw::Result<MyData> data = co_await receiver.Receive();
  if (!data.ok()) {
    PW_LOG_ERROR("Receiving failed: %s", data.status().str());
    co_return Status::Unavailable();
  }
  pw::Status sent = co_await sender.Send(std::move(*data));
  if (!sent.ok()) {
    PW_LOG_ERROR("Sending failed: %s", sent.str());
    co_return Status::Unavailable();
  }
  co_return OkStatus();
}

}  // namespace
// DOCSTAG: [pw_async2-examples-coro-injection]

#include "pw_allocator/testing.h"

namespace {

using ::pw::OkStatus;
using ::pw::allocator::test::AllocatorForTest;
using ::pw::async2::Context;
using ::pw::async2::Coro;
using ::pw::async2::CoroContext;
using ::pw::async2::Dispatcher;
using ::pw::async2::Pending;
using ::pw::async2::Poll;
using ::pw::async2::Ready;
using ::pw::async2::Task;

class ExpectCoroTask final : public Task {
 public:
  ExpectCoroTask(Coro<pw::Status>&& coro) : coro_(std::move(coro)) {}

 private:
  Poll<> DoPend(Context& cx) final {
    Poll<Status> result = coro_.Pend(cx);
    if (result.IsPending()) {
      return Pending();
    }
    EXPECT_EQ(*result, OkStatus());
    return Ready();
  }
  Coro<pw::Status> coro_;
};

TEST(CoroExample, ReturnsOk) {
  AllocatorForTest<256> alloc;
  CoroContext coro_cx(alloc);
  ExpectCoroTask task = ReceiveAndSend(coro_cx, MyReceiver(), MySender());
  Dispatcher dispatcher;
  dispatcher.Post(task);
  EXPECT_TRUE(dispatcher.RunUntilStalled().IsReady());
}

}  // namespace
