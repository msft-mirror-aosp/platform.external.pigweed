:tocdepth: 2

.. _docs-changelog:

=========
Changelog
=========

.. _docs-changelog-latest:

----------------------------
Jul 27, 2023 to Aug 11, 2023
----------------------------

.. changelog_highlights_start

Highlights (Jul 27, 2023 to Aug 11, 2023):

* We're prototyping a Pigweed extension for VS Code. Learn more at
  :ref:`docs-editors`.
* We added ``pw_toolchain_bazel``, a new LLVM toolchain for building with
  Bazel on macOS.
* We are working on many docs improvements in parallel: auto-generating ``rustdocs``
  for modules that support Rust
  (`example <https://pigweed.dev/rustdoc/pw_varint/>`_), refactoring the
  :ref:`module-pw_tokenizer` docs, migrating API references to Doxygen,
  fixing `longstanding docs site UI issues <https://issues.pigweed.dev/issues/292273650>`_,
  and more.

Please join us at the next Pigweed Live on Monday, Aug 14 1PM PST to
discuss these changes and anything else on your mind. Join our
`Discord <https://discord.gg/M9NSeTA>`_ and head over to the ``#pigweed-live``
channel to get a link to the video meeting.

.. changelog_highlights_end

Active SEEDs
============
Help shape the future of Pigweed! Please leave feedback on the following active RFCs (SEEDs):

* `SEED-0103: pw_protobuf Object Model <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/133971>`__
* `SEED-0104: display support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/150793>`__
* `SEED-0105: Add nested tokens and tokenized args to pw_tokenizer and pw_log <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154190>`__
* `SEED-0106: Project Template <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155430>`__
* `SEED-0107: Pigweed communications <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157090>`__
* `SEED-0108: Emulators Frontend <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158190>`__

Modules
=======

pw_alignment
------------
* `Fix typos <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163250>`__

pw_analog
---------
Long-term, all of our API references will be generated from header comments via
Doxygen. Short-term, we are starting to show header files directly within the
docs as a stopgap solution for helping Pigweed users get a sense of each
module's API. See :ref:`module-pw_analog` for an example.

* `Include header files as stopgap API reference <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161491>`__
  (issue `#293895312 <https://issues.pigweed.dev/issues/293895312>`__)

pw_base64
---------
We finished migrating the ``pw_random`` API reference to Doxygen.

* `Finish Doxygenifying the API reference <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162911>`__
* `Doxygenify the Encode() functions <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156532>`__

pw_boot_cortex_m
----------------
* `Allow explict target name <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159790>`__

pw_build
--------
We added a ``log_build_steps`` option to ``ProjectBuilder`` that enables you
to log all build step lines to your screen and logfiles.

* `Handle ProcessLookupError exceptions <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163710>`__
* `ProjectBuilder log build steps option <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162931>`__
* `Fix progress bar clear <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160791>`__

pw_cli
------
We polished tab completion support.

* `Zsh shell completion autoload <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160796>`__
* `Make pw_cli tab completion reusable <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160379>`__

pw_console
----------
We made copy-to-clipboard functionality more robust when running ``pw_console``
over SSH.

* `Set clipboard fallback methods <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/150238>`__

pw_containers
-------------
We updated :cpp:class:`filteredview` constructors and migrated the
``FilteredView`` API reference to Doxygen.

* `Doxygenify pw::containers::FilteredView <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160373>`__
* `Support copying the FilteredView predicate <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160372>`__

pw_docgen
---------
At the top of pages like :ref:`module-pw_tokenizer` there is a UI widget that
provides information about the module. Previously, this UI widget had links
to all the module's docs. This is no longer needed now that the site nav
automatically scrolls to the page you're on, which allows you to see the
module's other docs.

* `Remove the navbar from the module docs header widget <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162991>`__
  (issue `#292273650 <https://issues.pigweed.dev/issues/292273650>`__)

pw_env_setup
------------
We made Python setup more flexible.

* `Add clang_next.json <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163810>`__
  (issue `#295020927 <https://issues.pigweed.dev/issues/295020927>`__)
* `Pip installs from CIPD <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162093>`__
* `Include Python packages from CIPD <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162073>`__
* `Remove unused pep517 package <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162072>`__
* `Use more available Python 3.9 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161492>`__
  (issue `#292278707 <https://issues.pigweed.dev/issues/292278707>`__)
* `Update Bazel to 2@6.3.0.6 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161010>`__

pw_ide
------
We are prototyping a ``pw_ide`` extension for VS Code.

* `Restore stable clangd settings link <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/164011>`__
* `Add command to install prototype extension <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162412>`__
* `Prototype VS Code extension <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/151653>`__

pw_interrupt
------------
We added a backend for Xtensa processors.

* `Add backend for xtensa processors <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160031>`__
* `Tidy up target compatibility <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160650>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)

pw_log_zephyr
-------------
We encoded tokenized messages to ``pw::InlineString`` so that the output is
always null-terminated.

* `Fix null termination of Base64 messages <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163650>`__

pw_presubmit
------------
We increased
`LUCI <https://chromium.googlesource.com/infra/infra/+/main/doc/users/services/about_luci.md>`_
support and updated the ``#pragma once`` check to look for matching ``#ifndef``
and ``#define`` lines.

* `Fix overeager format_code matches <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162611>`__
* `Exclude vsix files from copyright <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163011>`__
* `Clarify unicode errors <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162993>`__
* `Upload coverage json to zoss <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162090>`__
  (issue `#279161371 <https://issues.pigweed.dev/issues/279161371>`__)
* `Add to context tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162311>`__
* `Add patchset to LuciTrigger <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162310>`__
* `Add helpers to LuciContext <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162091>`__
* `Update Python vendor wheel dir <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161514>`__
* `Add summaries to guard checks <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161391>`__
  (issue `#287529705 <https://issues.pigweed.dev/issues/287529705>`__)
* `Copy Python packages <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161490>`__
* `Add ifndef/define check <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/152173>`__
  (issue `#287529705 <https://issues.pigweed.dev/issues/287529705>`__)

pw_protobuf_compiler
--------------------
We continued work to ensure that the Python environment in Bazel is hermetic.

* `Use hermetic protoc <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162913>`__
  (issue `#294284927 <https://issues.pigweed.dev/issues/294284927>`__)
* `Move reference to python interpreter <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162932>`__
  (issue `#294414535 <https://issues.pigweed.dev/issues/294414535>`__)
* `Make nanopb hermetic <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162313>`__
  (issue `#293792686 <https://issues.pigweed.dev/issues/293792686>`__)

pw_python
---------
We fixed bugs related to ``requirements.txt`` files not getting found.

* `setup.sh requirements arg fix path <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/164430>`__
* `setup.sh arg spaces bug <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163510>`__

pw_random
---------
We continued migrating the ``pw_random`` API reference to Doxygen.

* `Doxygenify random.h <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163730>`__

pw_rpc
------
We made the Java client more robust.

* `Java client backwards compatibility <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/164515>`__
* `Avoid reflection in Java client <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162930>`__
  (issue `#293361955 <https://issues.pigweed.dev/issues/293361955>`__)
* `Use hermetic protoc <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162913>`__
  (issue `#294284927 <https://issues.pigweed.dev/issues/294284927>`__)
* `Improve Java client error message for missing parser() method <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159471>`__

pw_spi
------
We continued work on implementing a SPI responder interface.

* `Responder interface definition <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159230>`__

pw_status
---------
We fixed the nesting on a documentation section.

* `Promote Zephyr heading to h2 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160730>`__

pw_stream
---------
We added ``remaining()``, ``len()``, and ``position()`` methods to the
``Cursor`` wrapping in Rust.

* `Add infalible methods to Rust Cursor <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/164271>`__

pw_stream_shmem_mcuxpresso
--------------------------
We added the :ref:`module-pw_stream_shmem_mcuxpresso` backend for ``pw_stream``.

* `Add shared memory stream for NXP MCU cores <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160831>`__
  (issue `#294406620 <https://issues.pigweed.dev/issues/294406620>`__)

pw_sync_freertos
----------------
* `Fix ODR violation in tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160795>`__

pw_thread
---------
* `Fix test_thread_context typo and presubmit <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162770>`__

pw_tokenizer
------------
We added support for unaligned token databases and continued the
:ref:`seed-0102` update of the ``pw_tokenizer`` docs.

* `Separate API reference and how-to guide content <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163256>`__
* `Polish the sales pitch <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163571>`__
* `Support unaligned databases <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163333>`__
* `Move the basic overview into getting started <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163253>`__
* `Move the case study to guides.rst <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163255>`__
* `Restore info that get lost during the SEED-0102 migration <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163330>`__
* `Use the same tagline on every doc <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163332>`__
* `Replace savings table with flowchart <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158893>`__
* `Ignore string nonliteral warnings <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162092>`__

pw_toolchain
------------
We fixed a regression that made it harder to use Pigweed in an environment where
``pw_env_setup`` has not been run and fixed a bug related to incorrect Clang linking.

* `Optionally depend on pw_env_setup_CIPD_PIGWEED <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163790>`__
  (issue `#294886611 <https://issues.pigweed.dev/issues/294886611>`__)
* `Prefer start-group over whole-archive <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/150610>`__
  (issue `#285357895 <https://issues.pigweed.dev/issues/285357895>`__)

pw_toolchain_bazel
------------------
We added a an LLVM toolchain for building with Bazel on macOS.

* `LLVM toolchain for macOS Bazel build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157634>`__
  (issue `#291795899 <https://issues.pigweed.dev/issues/291795899>`__)

pw_trace_tokenized
------------------
We made tracing more robust.

* `Replace trace callback singletons with dep injection <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156912>`__

pw_transfer
-----------
We made integration tests more robust.

* `Fix use-after-destroy in integration test client <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163252>`__
  (issue `#294101325 <https://issues.pigweed.dev/issues/294101325>`__)
* `Fix legacy binary path <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162914>`__
  (issue `#294284927 <https://issues.pigweed.dev/issues/294284927>`__)
* `Mark linux-only Bazel tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162094>`__
  (issue `#294101325 <https://issues.pigweed.dev/issues/294101325>`__)

pw_web
------
We added support for storing user preferences in ``localStorage``.

* `Fix TypeScript warnings in web_serial_transport.ts <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/164591>`__
* `Add state for view number, search string, and columns visible <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161390>`__
* `Fix TypeScript warnings in transfer.ts <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162411>`__
* `Fix TypeScript warnings <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162095>`__
* `Remove dependency on 'crc' and 'buffer' NPM packages <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160830>`__

Build
=====
We made the Bazel system more hermetic and fixed an error related to not
finding the Java runtime.

* `Do not allow PATH leakage into Bazel build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162610>`__
  (issue `#294284927 <https://issues.pigweed.dev/issues/294284927>`__)
* `Use remote Java runtime for Bazel build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160793>`__
  (issue `#291791485 <https://issues.pigweed.dev/issues/291791485>`__)

Docs
====
We created a new doc (:ref:`docs-editors`) that explains how to improve Pigweed
support in various IDEs. We standardized how we present call-to-action buttons
on module homepages. See :ref:`module-pw_tokenizer` for an example. We fixed a
longstanding UI issue around the site nav not scrolling to the page that you're
currently on.

* `Add call-to-action buttons <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163331>`__
* `Update module items in site nav <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163251>`__
* `Add editor support doc <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/110261>`__
* `Delay nav scrolling to fix main content scrolling <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162990>`__
  (issue `#292273650 <https://issues.pigweed.dev/issues/292273650>`__)
* `Suggest editor configuration <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162710>`__
* `Scroll to the current page in the site nav <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/162410>`__
  (issue `#292273650 <https://issues.pigweed.dev/issues/292273650>`__)
* `Add changelog <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160170>`__
  (issue `#292247409 <https://issues.pigweed.dev/issues/292247409>`__)

SEEDs
=====
We created a UI widget to standardize how we present SEED status information.
See the start of :ref:`seed-0102` for an example.

* `Create Sphinx directive for metadata <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/161517>`__

Third party
===========

third_party/mbedtls
-------------------
* `3.3.0 compatibility <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160790>`__
  (issue `#293612945 <https://issues.pigweed.dev/issues/293612945>`__)

Miscellaneous
=============

OWNERS
------
* `Add kayce@ <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/163254>`__



----------------------------
Jul 13, 2023 to Jul 28, 2023
----------------------------
Highlights (Jul 13, 2023 to Jul 28, 2023):

* `SEED-0107: Pigweed Communications <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157090>`__,
  a proposal for an embedded-focused network protocol stack, is under
  discussion. Please review and provide your input!
* ``pw_cli`` now supports tab completion!
* A new UART Linux backend for ``pw_stream`` was added (``pw_stream_uart_linux``).

Active SEEDs
============
* `SEED-0103: pw_protobuf Object Model <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/133971>`__
* `SEED-0104: display support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/150793>`__
* `SEED-0105: Add nested tokens and tokenized args to pw_tokenizer and pw_log <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154190>`__
* `SEED-0106: Project Template <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155430>`__
* `SEED-0107: Pigweed communications <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157090>`__
* `SEED-0108: Emulators Frontend <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158190>`__

Modules
=======

pw_allocator
------------
We started migrating the ``pw_allocator`` API reference to Doxygen.

* `Doxygenify the freelist chunk methods <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155395>`__

pw_async
--------
We increased Bazel support.

* `Fill in bazel build rules <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156911>`__

pw_async_basic
--------------
We reduced logging noisiness.

* `Remove debug logging <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158193>`__

pw_base64
---------
We continued migrating the ``pw_base64`` API reference to Doxygen.

* `Doxygenify MaxDecodedSize() <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157091>`__

pw_bloat
--------
We improved the performance of label creation. One benchmark moved from 120
seconds to 0.02 seconds!

* `Cache and optimize label production <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159474>`__

pw_bluetooth
------------
Support for 3 events was added.

* `Add 3 Event packets & format hci.emb <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157663>`__

pw_build
--------
* `Fix progress bar clear <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160791>`__
* `Upstream build script fixes <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159473>`__
* `Add pw_test_info <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154551>`__
* `Upstream build script & presubmit runner <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/137130>`__
* `pw_watch: Redraw interval and bazel steps <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159490>`__
* `Avoid extra newlines for docs in generate_3p_gn <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/150233>`__
* `pip install GN args <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155270>`__
  (issue `#240701682 <https://issues.pigweed.dev/issues/240701682>`__)
* `pw_python_venv generate_hashes option <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157630>`__
  (issue `#292098416 <https://issues.pigweed.dev/issues/292098416>`__)

pw_build_mcuxpresso
-------------------
Some H3 elements in the ``pw_build_mcuxpresso`` docs were being incorrectly
rendered as H2.

* `Fix doc headings <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155570>`__

pw_chrono_freertos
------------------
We investigated what appeared to be a race condition but turned out to be an
unreliable FreeRTOS variable.

* `Update SystemTimer comments <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159231>`__
  (issue `#291346908 <https://issues.pigweed.dev/issues/291346908>`__)
* `Remove false callback precondition <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156091>`__
  (issue `#291346908 <https://issues.pigweed.dev/issues/291346908>`__)

pw_cli
------
``pw_cli`` now supports tab completion!

* `Zsh shell completion autoload <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160796>`__
* `Make pw_cli tab completion reusable <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160379>`__
* `Print tab completions for pw commands <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160032>`__
* `Fix logging msec timestamp format <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159930>`__

pw_console
----------
Communication errors are now handled more gracefully.

* `Detect comms errors in Python <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155814>`__

pw_containers
-------------
The flat map implementation and docs have been improved.

* `Doxygenify pw::containers::FilteredView <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160373>`__
* `Support copying the FilteredView predicate <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160372>`__
* `Improve FlatMap algorithm and filtered_view support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156652>`__
* `Improve FlatMap doc example <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156651>`__
* `Update FlatMap doc example so it compiles <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156650>`__

pw_digital_io
-------------
We continued migrating the API reference to Doxygen. An RPC service was added.

* `Doxygenify the interrupt handler methods <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154193>`__
* `Doxygenify Enable() and Disable() <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155817>`__
* `Add digital_io rpc service <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154974>`__

pw_digital_io_mcuxpresso
------------------------
We continued migrating the API reference to Doxygen. Support for a new RPC
service was added.

* `Remove unneeded constraints <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155394>`__

pw_docgen
---------
Support for auto-linking to Rust docs (when available) was added. We also
explained how to debug Pigweed's Sphinx extensions.

* `Add rustdoc linking support to pigweed-module tag <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159292>`__
* `Add extension debugging instructions <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156090>`__

pw_env_setup
------------
There were lots of updates around how the Pigweed environment uses Python.

* `pw_build: Disable pip version check <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160551>`__
* `Add docstrings to visitors <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159131>`__
* `Sort pigweed_environment.gni lines <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158892>`__
* `Mac and Windows Python requirements <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158912>`__
  (issue `#292098416 <https://issues.pigweed.dev/issues/292098416>`__)
* `Add more Python versions <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158891>`__
  (issue `#292278707 <https://issues.pigweed.dev/issues/292278707>`__)
* `Remove python.json from Bazel CIPD <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158911>`__
  (issue `#292585791 <https://issues.pigweed.dev/issues/292585791>`__)
* `Redirect variables from empty dirs <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158890>`__
  (issue `#292278707 <https://issues.pigweed.dev/issues/292278707>`__)
* `Split Python constraints per OS <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157657>`__
  (issue `#292278707 <https://issues.pigweed.dev/issues/292278707>`__)
* `Add --additional-cipd-file argument <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158170>`__
  (issue `#292280529 <https://issues.pigweed.dev/issues/292280529>`__)
* `Upgrade Python cryptography to 41.0.2 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157654>`__
* `Upgrade ipython to 8.12.2 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157653>`__
* `Upgrade PyYAML to 6.0.1 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157652>`__
* `Add Python constraints with hashes <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/153470>`__
  (issue `#287302102 <https://issues.pigweed.dev/issues/287302102>`__)
* `Bump pip and pip-tools <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156470>`__
* `Add coverage utilities <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155810>`__
  (issue `#279161371 <https://issues.pigweed.dev/issues/279161371>`__)

pw_fuzzer
---------
A fuzzer example was updated to more closely follow Pigweed code conventions.

* `Update fuzzers to use Pigweed domains <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/148337>`__

pw_hdlc
-------
Communication errors are now handled more gracefully.

* `Detect comms errors in Python <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155814>`__
* `Add target to Bazel build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157651>`__

pw_i2c
------
An RPC service was added. Docs and code comments were updated to use ``responder``
and ``initiator`` terminology consistently.

* `Standardize naming on initiator/responder <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159132>`__
* `Add i2c rpc service <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155250>`__

pw_i2c_mcuxpresso
-----------------
* `Allow for static initialization of initiator <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155790>`__
* `Add test to ensure compilation of module <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155390>`__

pw_ide
------
* `Support multiple comp DB search paths <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/144210>`__
  (issue `#280363633 <https://issues.pigweed.dev/issues/280363633>`__)

pw_interrupt
------------
Work continued on how facade backend selection works in Bazel.

* `Add backend for xtensa processors <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160031>`__
* `Tidy up target compatibility <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160650>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)
* `Remove cpu-based backend selection <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160380>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)
* `Add backend constraint setting <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160371>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)
* `Remove redundant Bazel targets <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154500>`__
  (issue `#290359233 <https://issues.pigweed.dev/issues/290359233>`__)

pw_log_rpc
----------
A docs section was added that explains how ``pw_log``, ``pw_log_tokenized``,
and ``pw_log_rpc`` interact with each other.

* `Explain relation to pw_log and pw_log_tokenized <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157231>`__

pw_package
----------
Raspberry Pi Pico and Zephyr support improved.

* `Add picotool package installer <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155791>`__
* `Handle windows Zephyr SDK setup <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157030>`__
* `Run Zephyr SDK setup.sh after syncing from CIPD <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156530>`__

pw_perf_test
------------
* `Remove redundant Bazel targets <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154498>`__
  (issue `#290359233 <https://issues.pigweed.dev/issues/290359233>`__)

pw_presubmit
------------
* `Add ifndef/define check <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/152173>`__
  (issue `#287529705 <https://issues.pigweed.dev/issues/287529705>`__)
* `Remove deprecated gn_docs_build step <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159291>`__
* `Fix issues with running docs_build twice <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159290>`__
* `Add Rust docs to docs site <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157656>`__

pw_protobuf_compiler
--------------------
* `Disable legacy namespace <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157232>`__
* `Transition to our own proto compiler rules <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157033>`__
  (issue `#234874064 <https://issues.pigweed.dev/issues/234874064>`__)
* `Allow external usage of macros <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155432>`__

pw_ring_buffer
--------------
``pw_ring_buffer`` now builds with ``-Wconversion`` enabled.

* `Conversion warning cleanups <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157430>`__
  (issue `#259746255 <https://issues.pigweed.dev/issues/259746255>`__)

pw_rpc
------
* `Create client call hook in Python client <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157870>`__
* `Provide way to populate response callbacks during tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156670>`__
* `Add Soong rule for pwpb echo service <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156270>`__

pw_rpc_transport
----------------
* `Add more Soong rules <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155035>`__

pw_rust
-------
We are preparing pigweed.dev to automatically link to auto-generated
Rust module documentation when available.

* `Add combined Rust doc support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157632>`__
* `Update @rust_crates sha <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155051>`__

pw_spi
------
We updated docs and code comments to use ``initiator`` and ``responder``
terminology consistently.

* `Standardize naming on initiator/responder <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159132>`__

pw_status
---------
* `Add Clone and Copy to Rust Error enum <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157093>`__

pw_stream
---------
We continued work on Rust support.

* `Fix Doxygen typo <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154732>`__
* `Add read_exact() an write_all() to Rust Read and Write traits <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157094>`__
* `Clean up rustdoc warnings <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157092>`__
* `Add Rust varint reading and writing support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156451>`__
* `Refactor Rust cursor to reduce monomorphization <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155391>`__
* `Add Rust integer reading support <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155053>`__
* `Move Rust Cursor to it's own sub-module <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155052>`__

pw_stream_uart_linux
--------------------
A new UART Linux backend for ``pw_stream`` was added.

* `Add stream for UART on Linux <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156910>`__

pw_sync
-------
C++ lock traits were added and used.

* `Improve Borrowable lock traits and annotations <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/153573>`__
  (issue `#261078330 <https://issues.pigweed.dev/issues/261078330>`__)
* `Add lock traits <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/153572>`__

pw_sync_freertos
----------------
* `Fix ODR violation in tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160795>`__

pw_sys_io
---------
* `Add android to alias as host system <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157871>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)
* `Add chromiumos to alias as host system <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155811>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)

pw_system
---------
* `Update IPython init API <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157872>`__
* `Remove redundant Bazel targets <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154497>`__
  (issue `#290359233 <https://issues.pigweed.dev/issues/290359233>`__)

pw_tokenizer
------------
We refactored the ``pw_tokenizer`` docs to adhere to :ref:`seed-0102`.

* `Update tagline, restore missing info, move sections <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158192>`__
* `Migrate the proto docs (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157655>`__
* `Remove stub sections and add guides link (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157631>`__
* `Migrate the custom macro example (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157032>`__
* `Migrate the Base64 docs (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156531>`__
* `Migrate token collision docs (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155818>`__
* `Migrate detokenization docs (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155815>`__
* `Migrate masking docs (SEED-0102) <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155812>`__

pw_toolchain
------------
The build system was modified to use relative paths to avoid unintentionally
relying on the path environment variable. Map file generation is now optional
to avoid generating potentially large map files when they're not needed.

* `Test trivially destructible class <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159232>`__
* `Make tools use relative paths <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159130>`__
  (issue `#290848929 <https://issues.pigweed.dev/issues/290848929>`__)
* `Support conditionally creating mapfiles <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157431>`__

pw_trace_tokenized
------------------
* `Replace singletons with dependency injection <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155813>`__
* `Remove redundant Bazel targets <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154499>`__
  (issue `#290359233 <https://issues.pigweed.dev/issues/290359233>`__)

pw_unit_test
------------
* `Update metadata test type for unit tests <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/154550>`__

pw_varint
---------
* `Update Rust API to return number of bytes written <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156450>`__

pw_watch
--------
We fixed an issue where builds were getting triggered when files were opened
or closed without modication.

* `Trigger build only on file modifications <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157633>`__

pw_web
------
* `Remove dependency on 'crc' and 'buffer' NPM packages <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160830>`__
* `Update theme token values and usage <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155970>`__
* `Add disconnect() method to WebSerialTransport <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156471>`__
* `Add docs section for log viewer component <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155050>`__

Build
=====

bazel
-----
* `Add host_backend_alias macro <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160550>`__
  (issue `#272090220 <https://issues.pigweed.dev/issues/272090220>`__)
* `Fix missing deps in some modules <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160376>`__
* `Support user bazelrc files <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160030>`__
* `Update rules_python to 0.24.0 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158913>`__
  (issue `#266950138 <https://issues.pigweed.dev/issues/266950138>`__)

build
-----
* `Use remote Java runtime for Bazel build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160793>`__
  (issue `#291791485 <https://issues.pigweed.dev/issues/291791485>`__)
* `Add Rust toolchain to Bazel macOS build <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159491>`__
  (issue `#291749888 <https://issues.pigweed.dev/issues/291749888>`__)
* `Mark linux-only Bazel build targets <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158191>`__

Targets
=======

targets/rp2040_pw_system
------------------------
Some of the Pico docs incorrectly referred to another hardware platform.

* `Fix references to STM32 <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157233>`__
  (issue `#286652309 <https://issues.pigweed.dev/issues/286652309>`__)

Language support
================

python
------
* `Remove setup.py files <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159472>`__

rust
----
* `Add rustdoc links for existing crates <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/159470>`__

OS support
==========

zephyr
------
* `Add project name to unit test root <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156850>`__
* `Add pigweed root as module <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156596>`__
* `Fix setup.sh call <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156591>`__

Docs
====
We added a feature grid to the homepage and fixed outdated info in various
docs.

* `pigweed.dev feature grid <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157658>`__
* `Mention SEED-0102 in module_structure.rst <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157234>`__
  (issue `#286477675 <https://issues.pigweed.dev/issues/286477675>`__)
* `Remove outdated Homebrew info in getting_started.rst <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157291>`__
  (issue `#287528787 <https://issues.pigweed.dev/issues/287528787>`__)
* `Fix "gn args" examples which reference pw_env_setup_PACKAGE_ROOT <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/156452>`__
* `Consolidate contributing docs in site nav <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/155816>`__

SEEDs
=====

SEED-0107
---------
* `Claim SEED number <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157031>`__

SEED-0108
---------
* `Claim SEED number <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/158171>`__

Third party
===========

third_party
-----------
* `Remove now unused rules_proto_grpc <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/157290>`__

third_party/mbedtls
-------------------
* `3.3.0 compatibility <https://pigweed-review.googlesource.com/c/pigweed/pigweed/+/160790>`__
  (issue `#293612945 <https://issues.pigweed.dev/issues/293612945>`__)
