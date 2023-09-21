Bazel
=====
Bazel is currently very experimental, and only builds for host and ARM Cortex-M
microcontrollers.

.. _module-pw_build-bazel-wrapper-rules:

Wrapper rules
-------------
The common configuration for Bazel for all modules is in the ``pigweed.bzl``
file. The built-in Bazel rules ``cc_binary``, ``cc_library``, and ``cc_test``
are wrapped with ``pw_cc_binary``, ``pw_cc_library``, and ``pw_cc_test``.
These wrappers add parameters to calls to the compiler and linker.

pw_linker_script
----------------
In addition to wrapping the built-in rules, Pigweed also provides a custom
rule for handling linker scripts with Bazel. e.g.

.. code-block:: python

  pw_linker_script(
    name = "some_linker_script",
    linker_script = ":some_configurable_linker_script.ld",
    defines = [
        "PW_BOOT_FLASH_BEGIN=0x08000200",
        "PW_BOOT_FLASH_SIZE=1024K",
        "PW_BOOT_HEAP_SIZE=112K",
        "PW_BOOT_MIN_STACK_SIZE=1K",
        "PW_BOOT_RAM_BEGIN=0x20000000",
        "PW_BOOT_RAM_SIZE=192K",
        "PW_BOOT_VECTOR_TABLE_BEGIN=0x08000000",
        "PW_BOOT_VECTOR_TABLE_SIZE=512",
    ],
  )

  pw_cc_binary(
    name = "some_binary",
    srcs = ["some_source.c"],
    additional_linker_inputs = [":some_linker_script"],
    linkopts = ["-T $(location :some_linker_script)"],
  )

pw_cc_facade
------------
In Bazel, a :ref:`facade <docs-module-structure-facades>` module has a few
components:

#. The **facade target**, i.e. the interface to the module. This is what
   *backend implementations* depend on to know what interface they're supposed
   to implement.  The facade is declared by creating a ``pw_cc_facade`` target,
   which is just a thin wrapper for ``cc_library``. For example,

   .. code-block:: python

     pw_cc_facade(
         name = "binary_semaphore_facade",
         # The header that constitues the facade.
         hdrs = [
             "public/pw_sync/binary_semaphore.h",
         ],
         includes = ["public"],
         # Dependencies of this header.
         deps = [
             "//pw_chrono:system_clock",
             "//pw_preprocessor",
         ],
     )

   .. note::
     As pure interfaces, ``pw_cc_facade`` targets should not include any source
     files. Backend-independent source files should be placed in the "library
     target" instead.

#. The **library target**, i.e. both the facade (interface) and backend
   (implementation). This is what *users of the module* depend on. It's a
   regular ``pw_cc_library`` that exposes the same headers as the facade, but
   has a dependency on the "backend label flag" (discussed next). It may also
   include some source files (if these are backend-independent). For example,

   .. code-block:: python

     pw_cc_library(
         name = "binary_semaphore",
         # A backend-independent source file.
         srcs = [
             "binary_semaphore.cc",
         ],
         # The same header as exposed by the facade.
         hdrs = [
             "public/pw_sync/binary_semaphore.h",
         ],
         deps = [
             # Dependencies of this header
             "//pw_chrono:system_clock",
             "//pw_preprocessor",
             # The backend, hidden behind a label_flag.
             "@pigweed_config//:pw_sync_binary_semaphore_backend",
         ],
     )

   .. note::
     You may be tempted to reduce duplication in the BUILD.bazel files and
     simply add the facade target to the ``deps`` of the library target,
     instead of re-declaring the facade's ``hdrs`` and ``deps``. *Do not do
     this!* It's a layering check violation: the facade headers provide the
     module's interface, and should be directly exposed by the target the users
     depend on.

#. The **backend label flag**. This is a `label_flag
   <https://bazel.build/extending/config#label-typed-build-settings>`_: a
   dependency edge in the build graph that can be overridden by downstream projects.
   For facades defined in upstream Pigweed, the ``label_flags`` are collected in
   the :ref:`pigweed_config <docs-build_system-bazel_configuration>`.

#. The **backend target** implements a particular backend for a facade. It's
   just a plain ``pw_cc_library``, with a dependency on the facade target. For example,

   .. code-block:: python

     pw_cc_library(
         name = "binary_semaphore",
         srcs = [
             "binary_semaphore.cc",
         ],
         hdrs = [
             "public/pw_sync_stl/binary_semaphore_inline.h",
             "public/pw_sync_stl/binary_semaphore_native.h",
             "public_overrides/pw_sync_backend/binary_semaphore_inline.h",
             "public_overrides/pw_sync_backend/binary_semaphore_native.h",
         ],
         includes = [
             "public",
             "public_overrides",
         ],
         deps = [
             # Dependencies of the backend's headers and sources.
             "//pw_assert",
             "//pw_chrono:system_clock",
             # A dependency on the facade target, which defines the interface
             # this backend target implements.
             "//pw_sync:binary_semaphore_facade",
         ],
     )

   If a project uses only one backend for a given facade, the backend label
   flag should point at that backend target.

#. The **facade constraint setting** and **backend constraint values**. Every
   facade has an associated `constraint setting
   <https://bazel.build/concepts/platforms#api-review>`_ (enum used in platform
   definition), and each backend for this facade has an associated
   ``constraint_value`` (enum value). Example:

   .. code-block:: python

     # //pw_sync/BUILD.bazel
     constraint_setting(
       name = "binary_semaphore_backend_constraint_setting",
     )

     # //pw_sync_stl/BUILD.bazel
     constraint_value(
       name = "binary_semaphore_backend",
       constraint_setting = "//pw_sync:binary_semaphore_backend_constraint_setting",
     )

     # //pw_sync_freertos/BUILD.bazel
     constraint_value(
       name = "binary_semaphore_backend",
       constraint_setting = "//pw_sync:binary_semaphore_backend_constraint_setting",
     )

   `Target platforms <https://bazel.build/extending/platforms>`_ for Pigweed
   projects should indicate which backend they select for each facade by
   listing the corresponding ``constraint_value`` in their definition. This can
   be used in a couple of ways:

   #.  It allows projects to switch between multiple backends based only on the
       `target platform <https://bazel.build/extending/platforms>`_ using a
       *backend multiplexer* (see below) instead of setting label flags in
       their ``.bazelrc``.

   #.  It allows tests or libraries that only support a particular backend to
       express this through the `target_compatible_with
       <https://bazel.build/reference/be/common-definitions#common.target_compatible_with>`_
       attribute. Bazel will use this to `automatically skip incompatible
       targets in wildcard builds
       <https://bazel.build/extending/platforms#skipping-incompatible-targets>`_.

#. The **backend multiplexer**. If a project uses more than one backend for a
   given facade (e.g., it uses different backends for host and embedded target
   builds), the backend label flag will point to a target that resolves to the
   correct backend based on the target platform. This will typically be an
   `alias <https://bazel.build/reference/be/general#alias>`_ with a ``select``
   statement mapping constraint values to the appropriate backend targets. For
   example,

   .. code-block:: python

     alias(
         name = "pw_sync_binary_semaphore_backend_multiplexer",
         actual = select({
             "//pw_sync_stl:binary_semaphore_backend": "@pigweed//pw_sync_stl:binary_semaphore",
             "//pw_sync_freertos:binary_semaphore_backend": "@pigweed//pw_sync_freertos:binary_semaphore_backend",
             # If we're building for a host OS, use the STL backend.
             "@platforms//os:macos": "@pigweed//pw_sync_stl:binary_semaphore",
             "@platforms//os:linux": "@pigweed//pw_sync_stl:binary_semaphore",
             "@platforms//os:windows": "@pigweed//pw_sync_stl:binary_semaphore",
             # Unless the target platform is the host platform, it must
             # explicitly specify which backend to use. The unspecified_backend
             # is not compatible with any platform; taking this branch will produce
             # an informative error.
             "//conditions:default": "@pigweed//pw_build:unspecified_backend",
         }),
     )

Toolchains and platforms
------------------------
Pigweed provides clang-based host toolchains for Linux and Mac Arm gcc
toolchain. The clang-based Linux and Arm gcc toolchains are entirely hermetic.
We don't currently provide a host toolchain for Windows.

