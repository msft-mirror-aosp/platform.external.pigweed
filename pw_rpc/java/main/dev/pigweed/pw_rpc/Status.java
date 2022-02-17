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

package dev.pigweed.pw_rpc;

/** Status object for RPC statuses. Must match gRPC's status codes. */
public enum Status {
  OK(0),
  CANCELLED(1),
  UNKNOWN(2),
  INVALID_ARGUMENT(3),
  DEADLINE_EXCEEDED(4),
  NOT_FOUND(5),
  ALREADY_EXISTS(6),
  PERMISSION_DENIED(7),
  UNAUTHENTICATED(16),
  RESOURCE_EXHAUSTED(8),
  FAILED_PRECONDITION(9),
  ABORTED(10),
  OUT_OF_RANGE(11),
  UNIMPLEMENTED(12),
  INTERNAL(13),
  UNAVAILABLE(14),
  DATA_LOSS(15);

  private final int code;

  private static final Status[] values = new Status[17];

  static {
    for (Status status : Status.values()) {
      values[status.code] = status;
    }
  }

  Status(int code) {
    this.code = code;
  }

  public final int code() {
    return code;
  }

  public final boolean ok() {
    return code == 0;
  }

  public static Status fromCode(int code) {
    return code >= 0 && code < values.length ? values[code] : null;
  }
}
