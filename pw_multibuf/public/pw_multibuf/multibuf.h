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
#pragma once

#include <tuple>

#include "pw_multibuf/chunk.h"
#include "pw_preprocessor/compiler.h"
#include "pw_status/status_with_size.h"

namespace pw::multibuf {

/// A buffer optimized for zero-copy data transfer.
///
/// A ``MultiBuf`` consists of multiple ``Chunk`` s of data.
class MultiBuf {
 public:
  class iterator;
  class const_iterator;
  class ChunkIterator;
  class ConstChunkIterator;
  class ChunkIterable;

  constexpr MultiBuf() : first_(nullptr) {}
  static MultiBuf FromChunk(OwnedChunk&& chunk) {
    MultiBuf buf;
    buf.first_ = std::move(chunk).Take();
    return buf;
  }
  MultiBuf(const MultiBuf&) = delete;
  MultiBuf& operator=(const MultiBuf&) = delete;

  // Disable maybe-uninitialized: this check fails erroniously on Windows GCC.
  PW_MODIFY_DIAGNOSTICS_PUSH();
  PW_MODIFY_DIAGNOSTIC_GCC(ignored, "-Wmaybe-uninitialized");
  constexpr MultiBuf(MultiBuf&& other) noexcept : first_(other.first_) {
    other.first_ = nullptr;
  }
  PW_MODIFY_DIAGNOSTICS_POP();

  MultiBuf& operator=(MultiBuf&& other) noexcept {
    Release();
    first_ = other.first_;
    other.first_ = nullptr;
    return *this;
  }

  /// Decrements the reference count on the underlying chunks of data and
  /// empties this ``MultiBuf`` so that ``size() == 0``.
  ///
  /// Does not modify the underlying data, but may cause it to be deallocated
  ///
  /// This method is equivalent to ``{ MultiBuf _unused = std::move(multibuf);
  /// }``
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  void Release() noexcept;

  /// This destructor will acquire a mutex and is not IRQ safe.
  ~MultiBuf() { Release(); }

  /// Returns the number of bytes in this container.
  ///
  /// This method's complexity is ``O(Chunks().size())``.
  [[nodiscard]] size_t size() const;

  /// Returns whether the container is empty (`size() == 0`).
  ///
  /// This method's complexity is ``O(Chunks().size())``, but will be more
  /// efficient than `size() == 0` in most cases.
  [[nodiscard]] bool empty() const;

  /// Returns if the `MultiBuf` is contiguous. A `MultiBuf` is contiguous if it
  /// is comprised of either:
  ///
  /// - one non-empty chunk,
  /// - only empty chunks, or
  /// - no chunks at all.
  [[nodiscard]] bool IsContiguous() const {
    return ContiguousSpan().has_value();
  }

  /// If the `MultiBuf` is contiguous, returns it as a span. The span will be
  /// empty if the `MultiBuf` is empty.
  ///
  /// A `MultiBuf` is contiguous if it is comprised of either:
  ///
  /// - one non-empty chunk,
  /// - only empty chunks, or
  /// - no chunks at all.
  std::optional<ByteSpan> ContiguousSpan() {
    auto result = std::as_const(*this).ContiguousSpan();
    if (result.has_value()) {
      return span(const_cast<std::byte*>(result->data()), result->size());
    }
    return std::nullopt;
  }
  std::optional<ConstByteSpan> ContiguousSpan() const;

  /// Returns an iterator pointing to the first byte of this ``MultiBuf`.
  iterator begin() { return iterator(first_); }
  /// Returns a const iterator pointing to the first byte of this ``MultiBuf`.
  const_iterator begin() const { return const_iterator(first_); }
  /// Returns a const iterator pointing to the first byte of this ``MultiBuf`.
  const_iterator cbegin() const { return const_iterator(first_); }

  /// Returns an iterator pointing to the end of this ``MultiBuf``.
  iterator end() { return iterator::end(); }
  /// Returns a const iterator pointing to the end of this ``MultiBuf``.
  const_iterator end() const { return const_iterator::end(); }
  /// Returns a const iterator pointing to the end of this ``MultiBuf``.
  const_iterator cend() const { return const_iterator::end(); }

  /// Attempts to add ``bytes_to_claim`` to the front of this buffer by
  /// advancing its range backwards in memory. Returns ``true`` if the operation
  /// succeeded.
  ///
  /// This will only succeed if the first ``Chunk`` in this buffer points to a
  /// section of a region that has unreferenced bytes preceeding it. See also
  /// ``Chunk::ClaimPrefix``.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  [[nodiscard]] bool ClaimPrefix(size_t bytes_to_claim);

  /// Attempts to add ``bytes_to_claim`` to the front of this buffer by
  /// advancing its range forwards in memory. Returns ``true`` if the operation
  /// succeeded.
  ///
  /// This will only succeed if the last ``Chunk`` in this buffer points to a
  /// section of a region that has unreferenced bytes following it. See also
  /// ``Chunk::ClaimSuffix``.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  [[nodiscard]] bool ClaimSuffix(size_t bytes_to_claim);

  /// Shrinks this handle to refer to the data beginning at offset
  /// ``bytes_to_discard``.
  ///
  /// Does not modify the underlying data. The discarded memory continues
  /// to be held by the underlying region as long as any ``Chunk``s exist within
  /// it. This allows the memory to be later reclaimed using ``ClaimPrefix``.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  void DiscardPrefix(size_t bytes_to_discard);

  /// Shrinks this handle to refer to data in the range ``begin..<end``.
  ///
  /// Does not modify the underlying data. The discarded memory continues
  /// to be held by the underlying region as long as any ``Chunk``s exist within
  /// it. This allows the memory to be later reclaimed using ``ClaimPrefix``
  /// or ``ClaimSuffix``.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  void Slice(size_t begin, size_t end);

  /// Shrinks this handle to refer to only the first ``len`` bytes.
  ///
  /// Does not modify the underlying data. The discarded memory continues
  /// to be held by the underlying region as long as any ``Chunk``s exist within
  /// it. This allows the memory to be later reclaimed using ``ClaimSuffix``.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  void Truncate(size_t len);

  /// Truncates the `MultiBuf` after the current iterator. All bytes following
  /// the iterator are removed.
  ///
  /// Does not modify the underlying data.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  void TruncateAfter(iterator pos);

  /// Attempts to shrink this handle to refer to the data beginning at
  /// offset ``bytes_to_take``, returning the first ``bytes_to_take`` bytes as
  /// a new ``MultiBuf``.
  ///
  /// If the inner call to ``AllocateChunkClass`` fails, this function
  /// will return ``std::nullopt` and this handle's span will not change.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  std::optional<MultiBuf> TakePrefix(size_t bytes_to_take);

  /// Attempts to shrink this handle to refer only the first
  /// ``len - bytes_to_take`` bytes, returning the last ``bytes_to_take``
  /// bytes as a new ``MultiBuf``.
  ///
  /// If the inner call to ``AllocateChunkClass`` fails, this function
  /// will return ``std::nullopt`` and this handle's span will not change.
  ///
  /// This method will acquire a mutex and is not IRQ safe.
  std::optional<MultiBuf> TakeSuffix(size_t bytes_to_take);

  /// Pushes ``front`` onto the front of this ``MultiBuf``.
  ///
  /// This operation does not move any data and is ``O(front.Chunks().size())``.
  void PushPrefix(MultiBuf&& front);

  /// Pushes ``tail`` onto the end of this ``MultiBuf``.
  ///
  /// This operation does not move any data and is ``O(Chunks().size())``.
  void PushSuffix(MultiBuf&& tail);

  /// Copies bytes from the multibuf into the provided buffer.
  ///
  /// @param[out] dest Destination into which to copy data from the `MultiBuf`.
  /// @param[in] position Offset in the `MultiBuf` from which to start.
  ///
  /// @returns @rst
  ///
  /// .. pw-status-codes::
  ///
  ///    OK: All bytes were copied into the desstination. The
  ///    :cpp:class:`pw::StatusWithSize` includes the number of bytes copied,
  ///    which is the size of the ``MultiBuf``.
  ///
  ///    RESOURCE_EXHAUSTED: Some bytes were copied, but the ``MultiBuf`` was
  ///    larger than the destination buffer. The :cpp:class:`pw::StatusWithSize`
  ///    includes the number of bytes copied.
  ///
  /// @endrst
  StatusWithSize CopyTo(ByteSpan dest, size_t position = 0) const;

  /// Copies bytes from the provided buffer into the multibuf.
  ///
  /// @param[in] source Data to copy into the `MultiBuf`.
  /// @param[in] position Offset in the `MultiBuf` from which to start.
  ///
  /// @returns @rst
  ///
  /// .. pw-status-codes::
  ///
  ///    OK: All bytes were copied. The :cpp:class:`pw::StatusWithSize` includes
  ///    the number of bytes copied, which is the size of the ``MultiBuf``.
  ///
  ///    RESOURCE_EXHAUSTED: Some bytes were copied, but the source was larger
  ///    than the destination. The :cpp:class:`pw::StatusWithSize` includes the
  ///    number of bytes copied.
  ///
  /// @endrst
  StatusWithSize CopyFrom(ConstByteSpan source, size_t position = 0) {
    return CopyFromAndOptionallyTruncate(source, position, /*truncate=*/false);
  }

  /// Copies bytes from the provided buffer into this `MultiBuf` and truncates
  /// it to the end of the copied data. This is a more efficient version of:
  /// @code{.cpp}
  ///
  ///   if (multibuf.CopyFrom(destination).ok()) {
  ///     multibuf.Truncate(destination.size());
  ///   }
  ///
  /// @endcode
  ///
  /// @param[in] source Data to copy into the `MultiBuf`.
  /// @param[in] position Offset in the `MultiBuf` from which to start.
  ///
  /// @returns @rst
  ///
  /// .. pw-status-codes::
  ///
  ///    OK: All bytes were copied and the `MultiBuf` was truncated. The
  ///    :cpp:class:`pw::StatusWithSize` includes the new `MultiBuf` size.
  ///
  ///    RESOURCE_EXHAUSTED: Some bytes were copied, but the source buffer was
  ///    larger than the ``MultiBuf``. The returned
  ///    :cpp:class:`pw::StatusWithSize` includes the number of bytes copied,
  ///    which is the size of the ``MultiBuf``.
  ///
  /// @endrst
  StatusWithSize CopyFromAndTruncate(ConstByteSpan source,
                                     size_t position = 0) {
    return CopyFromAndOptionallyTruncate(source, position, /*truncate=*/true);
  }

  ///////////////////////////////////////////////////////////////////
  //--------------------- Chunk manipulation ----------------------//
  ///////////////////////////////////////////////////////////////////

  /// Pushes ``Chunk`` onto the front of the ``MultiBuf``.
  ///
  /// This operation does not move any data and is ``O(1)``.
  void PushFrontChunk(OwnedChunk&& chunk);

  /// Pushes ``Chunk`` onto the end of the ``MultiBuf``.
  ///
  /// This operation does not move any data and is ``O(Chunks().size())``.
  void PushBackChunk(OwnedChunk&& chunk);

  /// Removes the first ``Chunk``.
  ///
  /// This operation does not move any data and is ``O(1)``.
  OwnedChunk TakeFrontChunk();

  /// Inserts ``chunk`` into the specified position in the ``MultiBuf``.
  ///
  /// This operation does not move any data and is ``O(Chunks().size())``.
  ///
  /// Returns an iterator pointing to the newly-inserted ``Chunk``.
  //
  // Implementation note: ``Chunks().size()`` should be remain relatively
  // small, but this could be made ``O(1)`` in the future by adding a ``prev``
  // pointer to the ``ChunkIterator``.
  ChunkIterator InsertChunk(ChunkIterator position, OwnedChunk&& chunk);

  /// Removes a ``Chunk`` from the specified position.
  ///
  /// This operation does not move any data and is ``O(Chunks().size())``.
  ///
  /// Returns an iterator pointing to the ``Chunk`` after the removed
  /// ``Chunk``, or ``Chunks().end()`` if this was the last ``Chunk`` in the
  /// ``MultiBuf``.
  //
  // Implementation note: ``Chunks().size()`` should be remain relatively
  // small, but this could be made ``O(1)`` in the future by adding a ``prev``
  // pointer to the ``ChunkIterator``.
  std::tuple<ChunkIterator, OwnedChunk> TakeChunk(ChunkIterator position);

  /// Returns an iterable container which yields the ``Chunk``s in this
  /// ``MultiBuf``.
  constexpr ChunkIterable Chunks() { return ChunkIterable(first_); }

  /// Returns an iterable container which yields the ``const Chunk``s in
  /// this ``MultiBuf``.
  constexpr const ChunkIterable Chunks() const { return ChunkIterable(first_); }

  /// Returns an iterator pointing to the first ``Chunk`` in this ``MultiBuf``.
  constexpr ChunkIterator ChunkBegin() { return ChunkIterator(first_); }
  /// Returns an iterator pointing to the end of the ``Chunk``s in this
  /// ``MultiBuf``.
  constexpr ChunkIterator ChunkEnd() { return ChunkIterator::end(); }
  /// Returns a const iterator pointing to the first ``Chunk`` in this
  /// ``MultiBuf``.
  constexpr ConstChunkIterator ConstChunkBegin() {
    return ConstChunkIterator(first_);
  }
  /// Returns a const iterator pointing to the end of the ``Chunk``s in this
  /// ``MultiBuf``.
  constexpr ConstChunkIterator ConstChunkEnd() {
    return ConstChunkIterator::end();
  }

  ///////////////////////////////////////////////////////////////////
  //--------------------- Iterator details ------------------------//
  ///////////////////////////////////////////////////////////////////

  using element_type = std::byte;
  using value_type = std::byte;
  using pointer = std::byte*;
  using const_pointer = const std::byte*;
  using reference = std::byte&;
  using const_reference = const std::byte&;
  using difference_type = std::ptrdiff_t;
  using size_type = std::size_t;

  /// A const ``std::forward_iterator`` over the bytes of a ``MultiBuf``.
  class const_iterator {
   public:
    using value_type = std::byte;
    using difference_type = std::ptrdiff_t;
    using reference = const std::byte&;
    using pointer = const std::byte*;
    using iterator_category = std::forward_iterator_tag;

    constexpr const_iterator() : chunk_(nullptr), byte_index_(0) {}

    reference operator*() const { return (*chunk_)[byte_index_]; }
    pointer operator->() const { return &(*chunk_)[byte_index_]; }

    const_iterator& operator++();
    const_iterator operator++(int) {
      const_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    const_iterator operator+(size_t rhs) const {
      const_iterator tmp = *this;
      tmp += rhs;
      return tmp;
    }
    const_iterator& operator+=(size_t advance);

    constexpr bool operator==(const const_iterator& other) const {
      return chunk_ == other.chunk_ && byte_index_ == other.byte_index_;
    }

    constexpr bool operator!=(const const_iterator& other) const {
      return !(*this == other);
    }

    /// Returns the current ``Chunk`` pointed to by this `iterator`.
    constexpr const Chunk* chunk() const { return chunk_; }

    /// Returns the index of the byte pointed to by this `iterator` within the
    /// current ``Chunk``.
    constexpr size_t byte_index() const { return byte_index_; }

   private:
    friend class MultiBuf;

    explicit constexpr const_iterator(const Chunk* chunk, size_t byte_index = 0)
        : chunk_(chunk), byte_index_(byte_index) {
      AdvanceToData();
    }

    static const_iterator end() { return const_iterator(nullptr); }

    constexpr void AdvanceToData() {
      while (chunk_ != nullptr && chunk_->empty()) {
        chunk_ = chunk_->next_in_buf_;
      }
    }

    const Chunk* chunk_;
    size_t byte_index_;
  };

  /// An ``std::forward_iterator`` over the bytes of a ``MultiBuf``.
  class iterator {
   public:
    using value_type = std::byte;
    using difference_type = std::ptrdiff_t;
    using reference = std::byte&;
    using pointer = std::byte*;
    using iterator_category = std::forward_iterator_tag;

    constexpr iterator() = default;

    reference operator*() const { return const_cast<std::byte&>(*const_iter_); }
    pointer operator->() const { return const_cast<std::byte*>(&*const_iter_); }

    iterator& operator++() {
      const_iter_++;
      return *this;
    }
    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    iterator operator+(size_t rhs) const {
      iterator tmp = *this;
      tmp += rhs;
      return tmp;
    }
    iterator& operator+=(size_t rhs) {
      const_iter_ += rhs;
      return *this;
    }
    constexpr bool operator==(const iterator& other) const {
      return const_iter_ == other.const_iter_;
    }
    constexpr bool operator!=(const iterator& other) const {
      return const_iter_ != other.const_iter_;
    }

    /// Returns the current ``Chunk`` pointed to by this `iterator`.
    constexpr Chunk* chunk() const {
      return const_cast<Chunk*>(const_iter_.chunk());
    }

    /// Returns the index of the byte pointed to by this `iterator` within the
    /// current ``Chunk``.
    constexpr size_t byte_index() const { return const_iter_.byte_index(); }

   private:
    friend class MultiBuf;

    explicit constexpr iterator(Chunk* chunk, size_t byte_index = 0)
        : const_iter_(chunk, byte_index) {}

    static iterator end() { return iterator(nullptr); }

    const_iterator const_iter_;
  };

  /// An iterable containing the ``Chunk`` s of a ``MultiBuf``.
  class ChunkIterable {
   public:
    using element_type = Chunk;
    using value_type = Chunk;
    using pointer = Chunk*;
    using reference = Chunk&;
    using const_pointer = const Chunk*;
    using difference_type = std::ptrdiff_t;
    using const_reference = const Chunk&;
    using size_type = std::size_t;

    /// Returns a reference to the first chunk.
    ///
    /// The behavior of this method is undefined when ``size() == 0``.
    Chunk& front() { return *first_; }
    const Chunk& front() const { return *first_; }

    /// Returns a reference to the final chunk.
    ///
    /// The behavior of this method is undefined when ``size() == 0``.
    ///
    /// NOTE: this method is ``O(size())``.
    Chunk& back();
    const Chunk& back() const;

    constexpr ChunkIterator begin() { return ChunkIterator(first_); }
    constexpr ConstChunkIterator begin() const { return cbegin(); }
    constexpr ConstChunkIterator cbegin() const {
      return ConstChunkIterator(first_);
    }
    constexpr ChunkIterator end() { return ChunkIterator::end(); }
    constexpr ConstChunkIterator end() const { return cend(); }
    constexpr ConstChunkIterator cend() const {
      return ConstChunkIterator::end();
    }

    /// Returns the number of ``Chunk``s in this iterable.
    size_t size() const;

   private:
    Chunk* first_ = nullptr;
    constexpr ChunkIterable(Chunk* chunk) : first_(chunk) {}
    friend class MultiBuf;
  };

  /// A ``std::forward_iterator`` over the ``Chunk``s of a ``MultiBuf``.
  class ChunkIterator {
   public:
    using value_type = Chunk;
    using difference_type = std::ptrdiff_t;
    using reference = Chunk&;
    using pointer = Chunk*;
    using iterator_category = std::forward_iterator_tag;

    constexpr ChunkIterator() = default;

    constexpr reference operator*() const { return *chunk_; }
    constexpr pointer operator->() const { return chunk_; }

    constexpr ChunkIterator& operator++() {
      chunk_ = chunk_->next_in_buf_;
      return *this;
    }

    constexpr ChunkIterator operator++(int) {
      ChunkIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr bool operator==(const ChunkIterator& other) const {
      return chunk_ == other.chunk_;
    }

    constexpr bool operator!=(const ChunkIterator& other) const {
      return chunk_ != other.chunk_;
    }

    constexpr Chunk* chunk() const { return chunk_; }

    constexpr operator ConstChunkIterator() const {
      return ConstChunkIterator(chunk_);
    }

   private:
    constexpr ChunkIterator(Chunk* chunk) : chunk_(chunk) {}
    static constexpr ChunkIterator end() { return ChunkIterator(nullptr); }
    Chunk* chunk_ = nullptr;
    friend class MultiBuf;
    friend class ChunkIterable;
  };

  /// A const ``std::forward_iterator`` over the ``Chunk``s of a ``MultiBuf``.
  class ConstChunkIterator {
   public:
    using value_type = const Chunk;
    using difference_type = std::ptrdiff_t;
    using reference = const Chunk&;
    using pointer = const Chunk*;
    using iterator_category = std::forward_iterator_tag;

    constexpr ConstChunkIterator() = default;

    constexpr reference operator*() const { return *chunk_; }
    constexpr pointer operator->() const { return chunk_; }

    constexpr ConstChunkIterator& operator++() {
      chunk_ = chunk_->next_in_buf_;
      return *this;
    }

    constexpr ConstChunkIterator operator++(int) {
      ConstChunkIterator tmp = *this;
      ++(*this);
      return tmp;
    }

    constexpr bool operator==(const ConstChunkIterator& other) const {
      return chunk_ == other.chunk_;
    }

    constexpr bool operator!=(const ConstChunkIterator& other) const {
      return chunk_ != other.chunk_;
    }

    constexpr const Chunk* chunk() const { return chunk_; }

   private:
    constexpr ConstChunkIterator(const Chunk* chunk) : chunk_(chunk) {}
    static constexpr ConstChunkIterator end() {
      return ConstChunkIterator(nullptr);
    }
    const Chunk* chunk_ = nullptr;
    friend class MultiBuf;
    friend class ChunkIterable;
    friend class ChunkIterator;
  };

 private:
  /// Returns the ``Chunk`` preceding ``chunk`` in this ``MultiBuf``.
  ///
  /// Requires that this ``MultiBuf`` is not empty, and that ``chunk``
  /// is either in ``MultiBuf`` or is ``nullptr``, in which case the last
  /// ``Chunk`` in ``MultiBuf`` will be returned.
  ///
  /// This operation is ``O(Chunks().size())``.
  Chunk* Previous(Chunk* chunk) const;

  StatusWithSize CopyFromAndOptionallyTruncate(ConstByteSpan source,
                                               size_t position,
                                               bool truncate);

  Chunk* first_;
};

}  // namespace pw::multibuf
