// Copyright 2023 The Pigweed Authors
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

#include "pw_async2/dispatcher.h"

#include "gtest/gtest.h"
#include "pw_containers/vector.h"

namespace pw::async2 {
namespace {

class MockTask : public Task {
 public:
  bool should_complete = false;
  int polled = 0;
  int destroyed = 0;
  Waker last_waker;

 private:
  Poll<> DoPend(Context& cx) override {
    ++polled;
    PW_ASYNC_STORE_WAKER(cx, last_waker, "MockTask is waiting for last_waker");
    if (should_complete) {
      return Ready();
    } else {
      return Pending();
    }
  }
  void DoDestroy() override { ++destroyed; }
};

class MockPendable {
 public:
  MockPendable(Poll<int> value) : value_(value) {}
  Poll<int> Pend(Context&) { return value_; }

 private:
  Poll<int> value_;
};

TEST(Dispatcher, RunUntilStalledPendsPostedTask) {
  MockTask task;
  task.should_complete = true;
  Dispatcher dispatcher;
  dispatcher.Post(task);
  EXPECT_TRUE(task.IsRegistered());
  EXPECT_TRUE(dispatcher.RunUntilStalled(task).IsReady());
  EXPECT_EQ(task.polled, 1);
  EXPECT_EQ(task.destroyed, 1);
  EXPECT_FALSE(task.IsRegistered());
}

TEST(Dispatcher, RunUntilStalledReturnsOnNotReady) {
  MockTask task;
  task.should_complete = false;
  Dispatcher dispatcher;
  dispatcher.Post(task);
  EXPECT_FALSE(dispatcher.RunUntilStalled(task).IsReady());
  EXPECT_EQ(task.polled, 1);
  EXPECT_EQ(task.destroyed, 0);
}

TEST(Dispatcher, RunUntilStalledDoesNotPendSleepingTask) {
  MockTask task;
  task.should_complete = false;
  Dispatcher dispatcher;
  dispatcher.Post(task);

  EXPECT_FALSE(dispatcher.RunUntilStalled(task).IsReady());
  EXPECT_EQ(task.polled, 1);
  EXPECT_EQ(task.destroyed, 0);

  task.should_complete = true;
  EXPECT_FALSE(dispatcher.RunUntilStalled(task).IsReady());
  EXPECT_EQ(task.polled, 1);
  EXPECT_EQ(task.destroyed, 0);

  std::move(task.last_waker).Wake();
  EXPECT_TRUE(dispatcher.RunUntilStalled(task).IsReady());
  EXPECT_EQ(task.polled, 2);
  EXPECT_EQ(task.destroyed, 1);
}

TEST(Dispatcher, RunUntilStalledWithNoTasksReturnsReady) {
  Dispatcher dispatcher;
  EXPECT_TRUE(dispatcher.RunUntilStalled().IsReady());
}

TEST(Dispatcher, RunToCompletionPendsMultipleTasks) {
  class CounterTask : public Task {
   public:
    CounterTask(pw::span<Waker> wakers,
                size_t this_waker_i,
                int* counter,
                int until)
        : counter_(counter),
          this_waker_i_(this_waker_i),
          until_(until),
          wakers_(wakers) {}
    int* counter_;
    size_t this_waker_i_;
    int until_;
    pw::span<Waker> wakers_;

   private:
    Poll<> DoPend(Context& cx) override {
      ++(*counter_);
      if (*counter_ >= until_) {
        for (auto& waker : wakers_) {
          std::move(waker).Wake();
        }
        return Ready();
      } else {
        PW_ASYNC_STORE_WAKER(cx,
                             wakers_[this_waker_i_],
                             "CounterTask is waiting for counter_ >= until_");
        return Pending();
      }
    }
  };

  int counter = 0;
  constexpr const int kNumTasks = 3;
  std::array<Waker, kNumTasks> wakers;
  CounterTask task_one(wakers, 0, &counter, kNumTasks);
  CounterTask task_two(wakers, 1, &counter, kNumTasks);
  CounterTask task_three(wakers, 2, &counter, kNumTasks);
  Dispatcher dispatcher;
  dispatcher.Post(task_one);
  dispatcher.Post(task_two);
  dispatcher.Post(task_three);
  EXPECT_TRUE(dispatcher.RunUntilStalled().IsReady());
  // We expect to see 5 total calls to `Pend`:
  // - two which increment counter and return pending
  // - one which increments the counter, returns complete, and wakes the
  //   others
  // - two which have woken back up and complete
  EXPECT_EQ(counter, 5);
}

TEST(Dispatcher, RunPendableUntilStalledReturnsOutputOnReady) {
  MockPendable pollable(Ready(5));
  Dispatcher dispatcher;
  Poll<int> result = dispatcher.RunPendableUntilStalled(pollable);
  EXPECT_EQ(result, Ready(5));
}

TEST(Dispatcher, RunPendableUntilStalledReturnsPending) {
  MockPendable pollable(Pending());
  Dispatcher dispatcher;
  Poll<int> result = dispatcher.RunPendableUntilStalled(pollable);
  EXPECT_EQ(result, Pending());
}

TEST(Dispathcer, RunPendableToCompletionReturnsOutput) {
  MockPendable pollable(Ready(5));
  Dispatcher dispatcher;
  int result = dispatcher.RunPendableToCompletion(pollable);
  EXPECT_EQ(result, 5);
}

TEST(Dispatcher, PostToDispatcherFromInsidePendSucceeds) {
  class TaskPoster : public Task {
   public:
    TaskPoster(Task& task_to_post) : task_to_post_(&task_to_post) {}

   private:
    Poll<> DoPend(Context& cx) override {
      cx.dispatcher().Post(*task_to_post_);
      return Ready();
    }
    Task* task_to_post_;
  };

  MockTask posted_task;
  posted_task.should_complete = true;
  TaskPoster task_poster(posted_task);

  Dispatcher dispatcher;
  dispatcher.Post(task_poster);
  EXPECT_TRUE(dispatcher.RunUntilStalled().IsReady());
  EXPECT_EQ(posted_task.polled, 1);
  EXPECT_EQ(posted_task.destroyed, 1);
}

TEST(Dispatcher, RunToCompletionPendsPostedTask) {
  MockTask task;
  task.should_complete = true;
  Dispatcher dispatcher;
  dispatcher.Post(task);
  dispatcher.RunToCompletion(task);
  EXPECT_EQ(task.polled, 1);
  EXPECT_EQ(task.destroyed, 1);
}

TEST(Dispatcher, RunToCompletionIgnoresDeregisteredTask) {
  Dispatcher dispatcher;
  MockTask task;
  task.should_complete = false;
  dispatcher.Post(task);
  EXPECT_TRUE(task.IsRegistered());
  task.Deregister();
  EXPECT_FALSE(task.IsRegistered());
  dispatcher.RunToCompletion();
  EXPECT_EQ(task.polled, 0);
  EXPECT_EQ(task.destroyed, 0);
}

}  // namespace
}  // namespace pw::async2
