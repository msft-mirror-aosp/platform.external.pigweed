.. _module-pw_allocator-api:

=============
API reference
=============
.. pigweed-module-subpage::
   :name: pw_allocator

This module provides the following:

- Generic allocator interfaces that can be injected into routines that need
  dynamic memory. These include :ref:`module-pw_allocator-api-allocator`, as
  well as the :ref:`module-pw_allocator-api-layout` type that is passed to it
  and the :ref:`module-pw_allocator-api-unique_ptr` returned from it.
- Concrete allocator implementations used to provide memory dynamically.
- "Forwarding" allocators, as described by
  :ref:`module-pw_allocator-design-forwarding`.
- Additional allocator utility classes. These are typically used by allocator
  implementers.
- Test utilities for testing allocator implementations. These are typically used
  by allocator implementers.

---------------
Core interfaces
---------------
This module defines several types as part of a generic interface for memory
users.

.. _module-pw_allocator-api-layout:

Layout
======
A request for memory includes a requested size and alignment as a ``Layout``:

.. doxygenclass:: pw::allocator::Layout
   :members:

.. _module-pw_allocator-api-allocator:

Allocator
=========
``Allocator`` is the most commonly used interface. It can be used to request
memory of different layouts:

.. doxygenclass:: pw::Allocator
   :members:

.. _module-pw_allocator-api-pool:

Pool
====
``Pool`` differs from ``Allocator`` in that it can be used to request memory of
a single, fixed layout:

.. doxygenclass:: pw::allocator::Pool
   :members:

.. _module-pw_allocator-api-deallocator:

Deallocator
===========
Both ``Allocator`` and ``Pool`` derive from and extend ``Deallocator``. This
type is intended for allocator implementers and not for module consumers.

.. doxygenclass:: pw::Deallocator
   :members:

.. _module-pw_allocator-api-capabilities:

Capabilities
============
Types deriving from ``MemoryResource`` can communicate about their optional
methods and behaviors using ``Capabilities``. This type is intended for
allocator implementers and not for module consumers.

.. doxygenclass:: pw::allocator::Capabilities
   :members:

.. _module-pw_allocator-api-unique_ptr:

UniquePtr
=========
The ``UniquePtr`` smart pointer type can be created by any type deriving from
``MemoryResource``.

.. doxygenclass:: pw::UniquePtr
   :members:

-------------------------
Allocator implementations
-------------------------
This module provides several concrete allocator implementations of the
:ref:`module-pw_allocator-api-allocator` interface:

.. _module-pw_allocator-api-block_allocator:

BlockAllocator
==============
.. doxygenclass:: pw::allocator::BlockAllocator
   :members:

.. _module-pw_allocator-api-first_fit_allocator:

FirstFitAllocator
-----------------
.. doxygenclass:: pw::allocator::FirstFitAllocator
   :members:

.. _module-pw_allocator-api-best_fit_allocator:

BestFitAllocator
----------------
.. doxygenclass:: pw::allocator::BestFitAllocator
   :members:

.. _module-pw_allocator-api-worst_fit_allocator:

WorstFitAllocator
-----------------
.. doxygenclass:: pw::allocator::WorstFitAllocator
   :members:

.. _module-pw_allocator-api-bucket_block_allocator:

BucketAllocator
===============
.. doxygenclass:: pw::allocator::BucketAllocator
   :members:

.. _module-pw_allocator-api-buddy_allocator:

BuddyAllocator
==============
.. doxygenclass:: pw::allocator::BuddyAllocator
   :members:

.. _module-pw_allocator-api-bump_allocator:

BumpAllocator
=============
.. doxygenclass:: pw::allocator::BumpAllocator
   :members:

.. _module-pw_allocator-api-chunk_pool:

ChunkPool
=========
.. doxygenclass:: pw::allocator::ChunkPool
   :members:

.. _module-pw_allocator-api-libc_allocator:

LibCAllocator
=============
.. doxygenclass:: pw::allocator::LibCAllocator
   :members:

.. _module-pw_allocator-api-null_allocator:

NullAllocator
=============
.. doxygenclass:: pw::allocator::NullAllocator
   :members:

.. _module-pw_allocator-api-typed_pool:

TypedPool
=========
.. doxygenclass:: pw::allocator::TypedPool
   :members:

---------------------
Forwarding Allocators
---------------------
This module provides several "forwarding" allocators, as described in
:ref:`module-pw_allocator-design-forwarding`.

.. _module-pw_allocator-api-allocator_as_pool:

AllocatorAsPool
===============
.. doxygenclass:: pw::allocator::AllocatorAsPool
   :members:

.. _module-pw_allocator-api-pmr_allocator:

PmrAllocator
============
.. doxygenclass:: pw::allocator::PmrAllocator
   :members:

.. _module-pw_allocator-api-fallback_allocator:

FallbackAllocator
=================
.. doxygenclass:: pw::allocator::FallbackAllocator
   :members:

.. _module-pw_allocator-api-synchronized_allocator:

SynchronizedAllocator
=====================
.. doxygenclass:: pw::allocator::SynchronizedAllocator
   :members:

.. _module-pw_allocator-api-tracking_allocator:

TrackingAllocator
=================
.. doxygenclass:: pw::allocator::TrackingAllocator
   :members:

---------------
Utility Classes
---------------
In addition to providing allocator implementations themselves, this module
includes some utility classes.

.. _module-pw_allocator-api-block:

Block interfaces
================
A block is an allocatable region of memory, and is the fundamental type managed
by several of the concrete allocator implementations. Blocks are defined
using several stateless "mix-in" interface types. These provide specific
functionality, while deferring the detailed representation of a block to a
derived type.

.. tip::
   Avoid converting pointers to allocations into ``Block`` instances, even if
   you know your memory is coming from a ``BlockAllocator``. Breaking the
   abstraction in this manner will limit your flexibility to change to a
   different allocator in the future.

.. TODO(b/378549332): Add a diagram of mix-in relationships.

BasicBlock
----------
.. doxygenclass:: pw::allocator::BasicBlock
   :members:

ContiguousBlock
---------------
.. doxygenclass:: pw::allocator::ContiguousBlock
   :members:

AllocatableBlock
----------------
.. doxygenclass:: pw::allocator::AllocatableBlock
   :members:

AlignableBlock
--------------
.. doxygenclass:: pw::allocator::AlignableBlock
   :members:

BlockWithLayout
---------------
.. doxygenclass:: pw::allocator::BlockWithLayout
   :members:

ForwardIterableBlock
--------------------
.. doxygenclass:: pw::allocator::ForwardIterableBlock
   :members:

ReverseIterableBlock
--------------------
.. doxygenclass:: pw::allocator::ReverseIterableBlock
   :members:

PoisonableBlock
---------------
.. doxygenclass:: pw::allocator::PoisonableBlock
   :members:

BlockResult
-----------
This type is not a block mix-in. It is used to communicate whether a method
succeeded, what block was produced or modified, and what side-effects the call
produced.

.. doxygenclass:: pw::allocator::BlockResult
   :members:

DetailedBlock
-------------
This type is not a block mix-in. It is an example of a block implementation that
uses the mix-ins above.

.. doxygenstruct:: pw::allocator::DetailedBlockParameters
   :members:

.. doxygenclass:: pw::allocator::DetailedBlockImpl
   :members:

.. _module-pw_allocator-api-bucket:

Bucket
======
Several block allocator implementations improve performance by managing buckets,
which are data structures that track free blocks. Several bucket implementations
are provided that trade off between performance and per-block space needed when
free.

FastSortedBucket
----------------
.. doxygenclass:: pw::allocator::FastSortedBucket
   :members:

ForwardSortedBucket
-------------------
.. doxygenclass:: pw::allocator::ForwardSortedBucket
   :members:

ReverseFastSortedBucket
-----------------------
.. doxygenclass:: pw::allocator::ReverseFastSortedBucket
   :members:

ReverseSortedBucket
-------------------
.. doxygenclass:: pw::allocator::ReverseSortedBucket
   :members:

SequencedBucket
---------------
.. doxygenclass:: pw::allocator::SequencedBucket
   :members:

UnorderedBucket
---------------
.. doxygenclass:: pw::allocator::UnorderedBucket
   :members:

.. _module-pw_allocator-api-metrics_adapter:

Metrics
=======
.. doxygenclass:: pw::allocator::internal::Metrics
   :members:

This class is templated on a ``MetricsType`` struct. See
:ref:`module-pw_allocator-design-metrics` for additional details on how the
struct, this class, and :ref:`module-pw_allocator-api-tracking_allocator`
interact.

Module consumers can define their own metrics structs using the
following macros:

.. doxygendefine:: PW_ALLOCATOR_METRICS_DECLARE
.. doxygendefine:: PW_ALLOCATOR_METRICS_ENABLE

.. _module-pw_allocator-api-fragmentation:

Fragmentation
=============
.. doxygenstruct:: pw::allocator::Fragmentation
   :members:

.. _module-pw_allocator-api-size_reporter:

SizeReporter
============
This module includes a utility class for generating size reports. It is
intended for allocator implementers and not for module consumers.

.. doxygenclass:: pw::allocator::SizeReporter
   :members:

Buffer management
=================
.. doxygenclass:: pw::allocator::WithBuffer
   :members:

------------
Test support
------------
This module includes test utilities for allocator implementers. These
facilitate writing unit tests and fuzz tests for both concrete and forwarding
allocator implementations. They are not intended to be used by module consumers.

.. _module-pw_allocator-api-allocator_for_test:

AllocatorForTest
================
.. doxygenclass:: pw::allocator::test::AllocatorForTest
   :members:

.. _module-pw_allocator-api-synchronized_allocator_for_test:

SynchronizedAllocatorForTest
============================
.. doxygenclass:: pw::allocator::test::SynchronizedAllocatorForTest
    :members:

.. _module-pw_allocator-api-test_harness:

TestHarness
===========
.. doxygenclass:: pw::allocator::test::TestHarness
   :members:

.. _module-pw_allocator-api-fuzzing_support:

FuzzTest support
================
.. doxygenfunction:: pw::allocator::test::ArbitraryRequest
.. doxygenfunction:: pw::allocator::test::ArbitraryRequests
.. doxygenfunction:: pw::allocator::test::MakeRequest
