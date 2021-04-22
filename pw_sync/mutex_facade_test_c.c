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

// These tests call the pw_sync module mutex API from C. The return values are
// checked in the main C++ tests.

#include <stdbool.h>

#include "pw_sync/mutex.h"

void pw_sync_Mutex_CallLock(pw_sync_Mutex* mutex) { pw_sync_Mutex_Lock(mutex); }

bool pw_sync_Mutex_CallTryLock(pw_sync_Mutex* mutex) {
  return pw_sync_Mutex_TryLock(mutex);
}

void pw_sync_Mutex_CallUnlock(pw_sync_Mutex* mutex) {
  pw_sync_Mutex_Unlock(mutex);
}
