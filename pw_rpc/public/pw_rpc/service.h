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
#pragma once

#include <cstdint>
#include <span>
#include <utility>

#include "pw_containers/intrusive_list.h"

namespace pw::rpc {
namespace internal {

class Method;

}  // namespace internal

// Base class for all RPC services. This cannot be instantiated directly; use a
// generated subclass instead.
class Service : public IntrusiveList<Service>::Item {
 public:
  uint32_t id() const { return id_; }

 protected:
  template <typename T>
  constexpr Service(uint32_t id, T&& methods)
      : id_(id), methods_(std::forward<T>(methods)) {}

 private:
  friend class Server;

  // Finds the method with the provided method_id. Returns nullptr if no match.
  const internal::Method* FindMethod(uint32_t method_id) const;

  uint32_t id_;
  std::span<const internal::Method> methods_;
};

}  // namespace pw::rpc
