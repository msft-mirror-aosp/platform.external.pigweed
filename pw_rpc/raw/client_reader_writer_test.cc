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

#include "pw_rpc/raw/client_reader_writer.h"

#include "gtest/gtest.h"
#include "pw_rpc/raw/client_testing.h"
#include "pw_rpc_test_protos/test.raw_rpc.pb.h"

namespace pw::rpc {
namespace {

using test::pw_rpc::raw::TestService;

void FailIfCalled(Status) { FAIL(); }
void FailIfOnNextCalled(ConstByteSpan) { FAIL(); }
void FailIfOnCompletedCalled(ConstByteSpan, Status) { FAIL(); }

TEST(RawClientReaderWriter, Move_InactiveToActive_EndsClientStream) {
  RawClientTestContext ctx;

  RawClientReaderWriter active_call =
      TestService::TestBidirectionalStreamRpc(ctx.client(),
                                              ctx.channel().id(),
                                              FailIfOnNextCalled,
                                              FailIfCalled,
                                              FailIfCalled);

  ASSERT_EQ(ctx.output().total_packets(), 1u);  // Sent the request

  RawClientReaderWriter inactive_call;

  active_call = std::move(inactive_call);

  EXPECT_EQ(ctx.output().total_packets(), 2u);  // Sent CLIENT_STREAM_END
  EXPECT_EQ(
      ctx.output()
          .client_stream_end_packets<TestService::TestBidirectionalStreamRpc>(),
      1u);

  EXPECT_FALSE(active_call.active());
  EXPECT_FALSE(inactive_call.active());
}

TEST(RawUnaryReceiver, Move_InactiveToActive_SilentlyCloses) {
  RawClientTestContext ctx;

  RawUnaryReceiver active_call =
      TestService::TestUnaryRpc(ctx.client(),
                                ctx.channel().id(),
                                {},
                                FailIfOnCompletedCalled,
                                FailIfCalled);

  ASSERT_EQ(ctx.output().total_packets(), 1u);  // Sent the request

  RawUnaryReceiver inactive_call;

  active_call = std::move(inactive_call);

  EXPECT_EQ(ctx.output().total_packets(), 1u);  // No more packets

  EXPECT_FALSE(active_call.active());
  EXPECT_FALSE(inactive_call.active());
}

TEST(RawUnaryReceiver, Move_ActiveToActive) {
  RawClientTestContext ctx;

  RawUnaryReceiver active_call_1 =
      TestService::TestUnaryRpc(ctx.client(), ctx.channel().id(), {});

  RawUnaryReceiver active_call_2 =
      TestService::TestAnotherUnaryRpc(ctx.client(), ctx.channel().id(), {});

  ASSERT_EQ(ctx.output().total_packets(), 2u);  // Sent the requests
  ASSERT_TRUE(active_call_1.active());
  ASSERT_TRUE(active_call_2.active());

  active_call_2 = std::move(active_call_1);

  EXPECT_EQ(ctx.output().total_packets(), 2u);  // No more packets

  EXPECT_FALSE(active_call_1.active());
  EXPECT_TRUE(active_call_2.active());
}

TEST(RawClientReaderWriter, NewCallCancelsPreviousAndCallsErrorCallback) {
  RawClientTestContext ctx;

  Status error;
  RawClientReaderWriter active_call_1 = TestService::TestBidirectionalStreamRpc(
      ctx.client(),
      ctx.channel().id(),
      FailIfOnNextCalled,
      FailIfCalled,
      [&error](Status status) { error = status; });

  ASSERT_TRUE(active_call_1.active());

  RawClientReaderWriter active_call_2 =
      TestService::TestBidirectionalStreamRpc(ctx.client(), ctx.channel().id());

  EXPECT_FALSE(active_call_1.active());
  EXPECT_TRUE(active_call_2.active());
  EXPECT_EQ(error, Status::Cancelled());
}

TEST(RawClientReader, NoClientStream_OutOfScope_SilentlyCloses) {
  RawClientTestContext ctx;

  {
    RawClientReader call = TestService::TestServerStreamRpc(ctx.client(),
                                                            ctx.channel().id(),
                                                            {},
                                                            FailIfOnNextCalled,
                                                            FailIfCalled,
                                                            FailIfCalled);
    ASSERT_EQ(ctx.output().total_packets(), 1u);  // Sent the request
  }

  EXPECT_EQ(ctx.output().total_packets(), 1u);  // No more packets
}

TEST(RawClientWriter, WithClientStream_OutOfScope_SendsClientStreamEnd) {
  RawClientTestContext ctx;

  {
    RawClientWriter call =
        TestService::TestClientStreamRpc(ctx.client(),
                                         ctx.channel().id(),
                                         FailIfOnCompletedCalled,
                                         FailIfCalled);
    ASSERT_EQ(ctx.output().total_packets(), 1u);  // Sent the request
  }

  EXPECT_EQ(ctx.output().total_packets(), 2u);  // Sent CLIENT_STREAM_END
  EXPECT_EQ(ctx.output()
                .client_stream_end_packets<TestService::TestClientStreamRpc>(),
            1u);
}

}  // namespace
}  // namespace pw::rpc
