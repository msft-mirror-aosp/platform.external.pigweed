// Copyright 2020 The Pigweed Authors
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

// These tests call the pw_sync module counting_semaphore API from C. The return
// values are checked in the main C++ tests.

#include <stdbool.h>

#include "pw_sync/counting_semaphore.h"

void pw_sync_CountingSemaphore_CallRelease(
    pw_sync_CountingSemaphore* semaphore) {
  pw_sync_CountingSemaphore_Release(semaphore);
}

void pw_sync_CountingSemaphore_CallReleaseNum(
    pw_sync_CountingSemaphore* semaphore, ptrdiff_t update) {
  pw_sync_CountingSemaphore_ReleaseNum(semaphore, update);
}

void pw_sync_CountingSemaphore_CallAcquire(
    pw_sync_CountingSemaphore* semaphore) {
  pw_sync_CountingSemaphore_Acquire(semaphore);
}

bool pw_sync_CountingSemaphore_CallTryAcquire(
    pw_sync_CountingSemaphore* semaphore) {
  return pw_sync_CountingSemaphore_TryAcquire(semaphore);
}

bool pw_sync_CountingSemaphore_CallTryAcquireFor(
    pw_sync_CountingSemaphore* semaphore,
    pw_chrono_SystemClock_Duration for_at_least) {
  return pw_sync_CountingSemaphore_TryAcquireFor(semaphore, for_at_least);
}

bool pw_sync_CountingSemaphore_CallTryAcquireUntil(
    pw_sync_CountingSemaphore* semaphore,
    pw_chrono_SystemClock_TimePoint until_at_least) {
  return pw_sync_CountingSemaphore_TryAcquireUntil(semaphore, until_at_least);
}

ptrdiff_t pw_sync_CountingSemaphore_CallMax(void) {
  return pw_sync_CountingSemaphore_Max();
}
