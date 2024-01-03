.. _module-pw_toolchain_bazel-api:

=============
API reference
=============
.. pigweed-module-subpage::
   :name: pw_toolchain_bazel
   :tagline: A modular toolkit for declaring C/C++ toolchains in Bazel

.. py:class:: pw_cc_toolchain

  This rule is the core of a C/C++ toolchain definition. Critically, it is
  intended to fully specify the following:

  * Which tools to use for various compile/link actions.
  * Which `well-known features <https://bazel.build/docs/cc-toolchain-config-reference#wellknown-features>`_
    are supported.
  * Which flags to apply to various actions.

   .. py:attribute:: action_configs
      :type: List[label]

      List of :py:class:`pw_cc_action_config` labels that bind tools to the
      appropriate actions. This is how Bazel knows which binaries to use when
      compiling, linking, or taking other actions like embedding data using
      objcopy.

   .. py:attribute:: action_config_flag_sets
      :type: List[label]

      List of flag sets to apply to the respective ``action_config``\s. The vast
      majority of labels listed here will point to :py:class:`pw_cc_flag_set`
      rules.

   .. py:attribute:: toolchain_identifier
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: host_system_name
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: target_system_name
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: target_cpu
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: target_libc
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: compiler
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: abi_version
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: abi_libc_version
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: cc_target_os
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: builtin_sysroot
      :type: str

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: cxx_builtin_include_directories
      :type: List[str]

      See  `cc_common.create_cc_toolchain_config_info() <https://bazel.build/rules/lib/toplevel/cc_common#create_cc_toolchain_config_info>`_\.

   .. py:attribute:: all_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      actions not associated with another ``*_files`` group.

      This differs from the native
      `cc_toolchain.all_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.all_files>`_
      attribute in that it automatically collects the contents of the other
      ``*_files`` attributes, which includes
      :py:attr:`pw_cc_tool.additional_files` from all
      :py:class:`pw_cc_action_config` rules associated with this toolchain.

   .. py:attribute:: ar_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      static link actions.

      This differs from the native
      `cc_toolchain.ar_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.ar_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: as_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      assembly actions.

      This differs from the native
      `cc_toolchain.as_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.as_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: compiler_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      C/C++ compile actions.

      This differs from the native
      `cc_toolchain.compiler_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.compiler_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: coverage_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      code coverage generation actions.

      This differs from the native
      `cc_toolchain.coverage_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.coverage_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: dwp_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      Dwarf package generation actions.

      This differs from the native
      `cc_toolchain.dwp_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.dwp_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: linker_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      link actions.

      This differs from the native
      `cc_toolchain.linker_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.linker_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: objcopy_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      objcopy actions.

      This differs from the native
      `cc_toolchain.objcopy_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.objcopy_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

   .. py:attribute:: strip_files
      :type: Label

      Label of a ``filegroup`` specifying additional files required to run
      strip actions.

      This differs from the native
      `cc_toolchain.strip_files <https://bazel.build/reference/be/c-cpp#cc_toolchain.strip_files>`_
      attribute in that it automatically collects
      :py:attr:`pw_cc_tool.additional_files` from the
      :py:class:`pw_cc_action_config` rules associated with these kinds of
      actions. Any ``filegroup`` specified here must also be enumerated in the
      ``filegroup`` listed in :py:attr:`pw_cc_toolchain.all_files`.

.. py:class:: pw_cc_flag_set

   Declares an ordered set of flags bound to a set of actions.

   Flag sets can be attached to a :py:class:`pw_cc_toolchain` via
   :py:attr:`pw_cc_toolchain.action_config_flag_sets`\.

   Examples:

   .. code-block:: py

      pw_cc_flag_set(
          name = "warnings_as_errors",
          flags = ["-Werror"],
      )

      pw_cc_flag_set(
          name = "layering_check",
          flag_groups = [
              ":strict_module_headers",
              ":dependent_module_map_files",
          ],
      )

   .. inclusive-language: disable

   Note: In the vast majority of cases, alphabetical sorting is not desirable
   for the :py:attr:`pw_cc_flag_set.flags` and
   :py:attr:`pw_cc_flag_set.flag_groups` attributes.
   `Buildifier <https://github.com/bazelbuild/buildtools/blob/master/buildifier/README.md>`_
   shouldn't ever try to sort these, but in the off chance it starts to these
   members should be listed as exceptions in the ``SortableDenylist``.

   .. inclusive-language: enable

   .. py:attribute:: actions
      :type: List[str]

      A list of action names that this flag set applies to.

      .. inclusive-language: disable

      Valid choices are listed at
      `@rules_cc//cc:action_names.bzl <https://github.com/bazelbuild/bazel/blob/master/tools/build_defs/cc/action_names.bzl>`_\.

      .. inclusive-language: enable

      It is possible for some needed action names to not be enumerated in this list,
      so there is not rigid validation for these strings. Prefer using constants
      rather than manually typing action names.

   .. py:attribute:: flags
      :type: List[str]

      Flags that should be applied to the specified actions.

      These are evaluated in order, with earlier flags appearing earlier in the
      invocation of the underlying tool. If you need expansion logic, prefer
      enumerating flags in a :py:class:`pw_cc_flag_group` or create a custom
      rule that provides ``FlagGroupInfo``.

      Note: :py:attr:`pw_cc_flag_set.flags` and
      :py:attr:`pw_cc_flag_set.flag_groups` are mutually exclusive.

   .. py:attribute:: flag_groups
      :type: List[label]

      Labels pointing to :py:class:`pw_cc_flag_group` rules.

      This is intended to be compatible with any other rules that provide
      ``FlagGroupInfo``. These are evaluated in order, with earlier flag groups
      appearing earlier in the invocation of the underlying tool.

      Note: :py:attr:`pw_cc_flag_set.flag_groups` and
      :py:attr:`pw_cc_flag_set.flags` are mutually exclusive.

.. py:class:: pw_cc_flag_group

   Declares an (optionally parametric) ordered set of flags.

   :py:class:`pw_cc_flag_group` rules are expected to be consumed exclusively by
   :py:class:`pw_cc_flag_set` rules. Though simple lists of flags can be
   expressed by populating ``flags`` on a :py:class:`pw_cc_flag_set`,
   :py:class:`pw_cc_flag_group` provides additional power in the following two
   ways:

    1. Iteration and conditional expansion. Using
       :py:attr:`pw_cc_flag_group.iterate_over`,
       :py:attr:`pw_cc_flag_group.expand_if_available`\, and
       :py:attr:`pw_cc_flag_group.expand_if_not_available`\, more complex
       flag expressions can be made. This is critical for implementing things
       like the ``libraries_to_link`` feature, where library names are
       transformed into flags that end up in the final link invocation.

       Note: ``expand_if_equal``, ``expand_if_true``, and ``expand_if_false``
       are not yet supported.

    2. Flags are tool-independent. A :py:class:`pw_cc_flag_group` expresses
       ordered flags that may be reused across various
       :py:class:`pw_cc_flag_set` rules. This is useful for cases where multiple
       :py:class:`pw_cc_flag_set` rules must be created to implement a feature
       for which flags are slightly different depending on the action (e.g.
       compile vs link). Common flags can be expressed in a shared
       :py:class:`pw_cc_flag_group`, and the differences can be relegated to
       separate :py:class:`pw_cc_flag_group` instances.

   Examples:

   .. code-block:: py

      pw_cc_flag_group(
          name = "user_compile_flag_expansion",
          flags = ["%{user_compile_flags}"],
          iterate_over = "user_compile_flags",
          expand_if_available = "user_compile_flags",
      )

      # This flag_group might be referenced from various FDO-related
      # `pw_cc_flag_set` rules. More importantly, the flag sets pulling this in
      # may apply to different sets of actions.
      pw_cc_flag_group(
          name = "fdo_profile_correction",
          flags = ["-fprofile-correction"],
          expand_if_available = "fdo_profile_path",
      )

   .. py:attribute:: flags
      :type: List[str]

      List of flags provided by this rule.

      For extremely complex expressions of flags that require nested flag groups
      with multiple layers of expansion, prefer creating a custom rule in
      `Starlark <https://bazel.build/rules/language>`_ that provides
      ``FlagGroupInfo`` or ``FlagSetInfo``.


   .. py:attribute:: iterate_over
      :type: str

      Expands :py:attr:`pw_cc_flag_group.flags` for items in the named list.

      Toolchain actions have various variables accessible as names that can be
      used to guide flag expansions. For variables that are lists,
      :py:attr:`pw_cc_flag_group.iterate_over` must be used to expand the list into a series of flags.

      Note that :py:attr:`pw_cc_flag_group.iterate_over` is the string name of a
      build variable, and not an actual list. Valid options are listed in the
      `C++ Toolchain Configuration <https://bazel.build/docs/cc-toolchain-config-reference#cctoolchainconfiginfo-build-variables>`_
      reference.



      Note that the flag expansion stamps out the entire list of flags in
      :py:attr:`pw_cc_flag_group.flags` once for each item in the list.

      Example:

      .. code-block:: py

         # Expands each path in ``system_include_paths`` to a series of
         # ``-isystem`` includes.
         #
         # Example input:
         #     system_include_paths = ["/usr/local/include", "/usr/include"]
         #
         # Expected result:
         #     "-isystem /usr/local/include -isystem /usr/include"
         pw_cc_flag_group(
             name = "system_include_paths",
             flags = ["-isystem", "%{system_include_paths}"],
             iterate_over = "system_include_paths",
         )

   .. py:attribute:: expand_if_available
      :type: str

      Expands the expression in :py:attr:`pw_cc_flag_group.flags` if the
      specified build variable is set.

   .. py:attribute:: expand_if_not_available
      :type: str

      Expands the expression in :py:attr:`pw_cc_flag_group.flags` if the
      specified build variable is **NOT** set.

.. py:class:: pw_cc_tool

   Declares a singular tool that can be bound to action configs.

   :py:class:`pw_cc_tool` rules are intended to be consumed exclusively by
   :py:class:`pw_cc_action_config` rules. These rules declare an underlying tool
   that can be used to fulfill various actions. Many actions may reuse a shared
   tool.

   Note: ``with_features`` is not yet supported.

   Examples:

   .. code-block:: py

      # A project-provided tool.
      pw_cc_tool(
          name = "clang_tool",
          tool = "@llvm_toolchain//:bin/clang",
      )

      # A tool expected to be preinstalled on a user's machine.
      pw_cc_tool(
          name = "clang_tool",
          path = "/usr/bin/clang",
      )

   .. py:attribute:: tool
      :type: label

      The underlying tool that this rule represents.

      This attribute is a label rather than a simple file path. This means that
      the file must be referenced relative to the BUILD file that exports it.
      For example:

      .. code-block:: none

         @llvm_toolchain//:bin/clang
         ^              ^  ^

      Where:

      * ``@llvm_toolchain`` is the repository.
      * ``//`` is the directory of the BUILD file that exports the file of
        interest.
      * ``bin/clang`` is the path of the actual binary relative to the BUILD
        file of interest.

      Note: :py:attr:`pw_cc_tool.tool` and :py:attr:`pw_cc_tool.path` are
      mutually exclusive.

   .. py:attribute:: path
      :type: Path

      An absolute path to a binary to use for this tool.

      Relative paths are also supported, but they are relative to the
      :py:class:`pw_cc_toolchain` that uses this tool rather than relative to
      this :py:class:`pw_cc_tool` rule.

      Note: :py:attr:`pw_cc_tool.path` and :py:attr:`pw_cc_tool.tool` are
      mutually exclusive.

      .. admonition:: Note
         :class: warning

         This method of listing a tool is NOT recommended, and is provided as an
         escape hatch for edge cases. Prefer using :py:attr:`pw_cc_tool.tool`
         whenever possible.

   .. py:attribute:: execution_requirements
      :type: List[str]

      A list of strings that provide hints for execution environment
      compatibility (e.g. ``requires-darwin``).

   .. py:attribute:: additional_files
      :type: List[label]

      Additional files that are required for this tool to correctly operate.
      These files are propagated up to the :py:class:`pw_cc_toolchain` so you
      typically won't need to explicitly specify the ``*_files`` attributes
      on a :py:class:`pw_cc_toolchain`.


.. py:class:: pw_cc_action_config

   Declares the configuration and selection of `pw_cc_tool` rules.

   Action configs are bound to a toolchain through `action_configs`, and are the
   driving mechanism for controlling toolchain tool invocation/behavior.

   Action configs define three key things:

   * Which tools to invoke for a given type of action.
   * Tool features and compatibility.
   * :py:class:`pw_cc_flag_set`\s that are unconditionally bound to a tool
     invocation.

   Examples:

   .. code-block:: py

      pw_cc_action_config(
          name = "ar",
          action_names = ALL_AR_ACTIONS,
          implies = [
              "archiver_flags",
              "linker_param_file",
          ],
          tools = [":ar_tool"],
      )

      pw_cc_action_config(
          name = "clang",
          action_names = ALL_ASM_ACTIONS + ALL_C_COMPILER_ACTIONS,
          tools = [":clang_tool"],
      )

   .. py:attribute:: action_names
      :type: List[str]

      A list of action names to apply this action to.

      .. inclusive-language: disable

      Valid choices are listed at
      `@rules_cc//cc:action_names.bzl <https://github.com/bazelbuild/bazel/blob/master/tools/build_defs/cc/action_names.bzl>`_\.

      .. inclusive-language: enable

      It is possible for some needed action names to not be enumerated in this list,
      so there is not rigid validation for these strings. Prefer using constants
      rather than manually typing action names.

   .. py:attribute:: enabled
      :type: bool

      Whether or not this action config is enabled by default.

      .. admonition:: Note

         This defaults to ``True`` since it's assumed that most listed action
         configs will be enabled and used by default. This is the opposite of
         Bazel's native default.

   .. py:attribute:: tools
      :type: List[label]

      The :py:class:`pw_cc_tool` to use for the specified actions.

      If multiple tools are specified, the first tool that has ``with_features``
      that satisfy the currently enabled feature set is used.

   .. py:attribute:: flag_sets
      :type: List[label]

      Labels that point to :py:class:`pw_cc_flag_set`\s that are unconditionally
      bound to the specified actions.

      .. admonition:: Note

         The flags in the :py:class:`pw_cc_flag_set` are only bound to matching
         action names. If an action is listed in this rule's
         :py:attr:`pw_cc_action_config.action_names`,
         but is NOT listed in the :py:class:`pw_cc_flag_set`\'s
         :py:attr:`pw_cc_flag_set.actions`, the flag will not be applied to that
         action.

   .. py:attribute:: implies
      :type: List[str]

      Names of features that should be automatically enabled when this tool is
      used.

      .. admonition:: Note
         :class: warning

         If this action config implies an unknown feature, this action config
         will silently be disabled. This behavior is native to Bazel itself, and
         there's no way to detect this and emit an error instead. For this
         reason, be very cautious when listing implied features!
