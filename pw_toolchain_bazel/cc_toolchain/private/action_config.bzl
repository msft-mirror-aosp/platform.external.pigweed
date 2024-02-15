# Copyright 2023 The Pigweed Authors
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
#     https://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
"""Implementation of pw_cc_action_config and pw_cc_tool."""

load(
    ":providers.bzl",
    "PwActionConfigInfo",
    "PwActionConfigSetInfo",
    "PwActionNameSetInfo",
    "PwFeatureConstraintInfo",
    "PwFeatureSetInfo",
    "PwFlagSetInfo",
    "PwToolInfo",
)

def _bin_to_files(target, extra_files = []):
    if not target:
        return depset(extra_files)
    info = target[DefaultInfo]
    exe = info.files_to_run.executable
    if not exe:
        return depset(extra_files)
    return depset(
        [exe] + extra_files,
        transitive = [info.files, info.data_runfiles.files],
    )

def _pw_cc_tool_impl(ctx):
    """Implementation for pw_cc_tool."""

    # Remaps empty strings to `None` to match behavior of the default values.
    exe = ctx.executable.tool or None
    path = ctx.attr.path or None

    if (exe == None) == (path == None):
        fail("Exactly one of tool and path must be provided. Prefer tool")

    tool = PwToolInfo(
        label = ctx.label,
        exe = exe,
        path = path,
        files = _bin_to_files(ctx.attr.tool, ctx.files.additional_files),
        requires_any_of = tuple([fc[PwFeatureConstraintInfo] for fc in ctx.attr.requires_any_of]),
        execution_requirements = tuple(ctx.attr.execution_requirements),
    )

    return [
        tool,
        DefaultInfo(files = tool.files),
    ]

pw_cc_tool = rule(
    implementation = _pw_cc_tool_impl,
    attrs = {
        "tool": attr.label(
            allow_files = True,
            executable = True,
            cfg = "exec",
            doc = """The underlying tool that this rule represents.

This attribute is a label rather than a simple file path. This means that the
file must be referenced relative to the BUILD file that exports it. For example:

    @llvm_toolchain//:bin/clang
    ^              ^  ^
    Where:

    * `@llvm_toolchain` is the repository.
    * `//` is the directory of the BUILD file that exports the file of interest.
    * `bin/clang` is the path of the actual binary relative to the BUILD file of
      interest.
""",
        ),
        "path": attr.string(
            doc = """An absolute path to a binary to use for this tool.

Relative paths are also supported, but they are relative to the
`pw_cc_toolchain` that uses this tool rather than relative to this `pw_cc_tool`
rule.

WARNING: This method of listing a tool is NOT recommended, and is provided as an
escape hatch for edge cases. Prefer using `tool` whenever possible.
""",
        ),
        "execution_requirements": attr.string_list(
            doc = "A list of strings that provide hints for execution environment compatibility (e.g. `requires-darwin`).",
        ),
        "requires_any_of": attr.label_list(
            providers = [PwFeatureConstraintInfo],
            doc = """This will be enabled when any of the constraints are met.

If omitted, this tool will be enabled unconditionally.
""",
        ),
        "additional_files": attr.label_list(
            allow_files = True,
            doc = """Additional files that are required for this tool to correctly operate.

These files are propagated up to the `pw_cc_toolchain` so you typically won't
need to explicitly specify the `*_files` attributes on a `pw_cc_toolchain`.
""",
        ),
    },
    provides = [PwToolInfo, DefaultInfo],
    doc = """Declares a singular tool that can be bound to action configs.

`pw_cc_tool` rules are intended to be consumed exclusively by
`pw_cc_action_config` rules. These rules declare an underlying tool that can
be used to fulfill various actions. Many actions may reuse a shared tool.

Example:

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
""",
)

def _generate_action_config(ctx, action_name, **kwargs):
    flag_sets = []
    for fs in ctx.attr.flag_sets:
        provided_fs = fs[PwFlagSetInfo]
        if action_name in provided_fs.actions:
            flag_sets.append(provided_fs)

    return PwActionConfigInfo(
        action_name = action_name,
        flag_sets = tuple(flag_sets),
        **kwargs
    )

def _pw_cc_action_config_impl(ctx):
    """Implementation for pw_cc_tool."""
    if not ctx.attr.tools:
        fail("Action configs are not valid unless they specify at least one `pw_cc_tool` in `tools`")

    action_names = depset(transitive = [
        action[PwActionNameSetInfo].actions
        for action in ctx.attr.action_names
    ]).to_list()
    if not action_names:
        fail("Action configs are not valid unless they specify at least one action name in `action_names`")

    # Check that the listed flag sets apply to at least one action in this group
    # of action configs.
    for fs in ctx.attr.flag_sets:
        provided_fs = fs[PwFlagSetInfo]
        flag_set_applies = False
        for action in action_names:
            if action in provided_fs.actions:
                flag_set_applies = True
        if not flag_set_applies:
            fail("{} listed as a flag set to apply to {}, but none of the actions match".format(
                fs.label,
                ctx.label,
            ))
    tools = tuple([tool[PwToolInfo] for tool in ctx.attr.tools])

    files = depset(transitive = [dep[DefaultInfo].files for dep in ctx.attr.tools])
    common_kwargs = dict(
        label = ctx.label,
        tools = tools,
        implies_features = depset(transitive = [
            ft_set[PwFeatureSetInfo].features
            for ft_set in ctx.attr.implies
        ]),
        implies_action_configs = depset([]),
        enabled = ctx.attr.enabled,
        files = files,
    )
    action_configs = [
        _generate_action_config(ctx, action, **common_kwargs)
        for action in action_names
    ]

    return [
        PwActionConfigSetInfo(
            label = ctx.label,
            action_configs = depset(action_configs),
        ),
        DefaultInfo(files = files),
    ]

pw_cc_action_config = rule(
    implementation = _pw_cc_action_config_impl,
    attrs = {
        "action_names": attr.label_list(
            providers = [PwActionNameSetInfo],
            mandatory = True,
            doc = """A list of action names to apply this action to.

See @pw_toolchain//actions:all for valid options.
""",
        ),
        "enabled": attr.bool(
            default = True,
            doc = """Whether or not this action config is enabled by default.

Note: This defaults to `True` since it's assumed that most listed action configs
will be enabled and used by default. This is the opposite of Bazel's native
default.
""",
        ),
        "tools": attr.label_list(
            mandatory = True,
            providers = [PwToolInfo],
            doc = """The `pw_cc_tool` to use for the specified actions.

If multiple tools are specified, the first tool that has `with_features` that
satisfy the currently enabled feature set is used.
""",
        ),
        "flag_sets": attr.label_list(
            providers = [PwFlagSetInfo],
            doc = """Labels that point to `pw_cc_flag_set`s that are
unconditionally bound to the specified actions.

Note: The flags in the `pw_cc_flag_set` are only bound to matching action names.
If an action is listed in this rule's `action_names`, but is NOT listed in the
`pw_cc_flag_set`'s `actions`, the flag will not be applied to that action.
""",
        ),
        "implies": attr.label_list(
            providers = [PwFeatureSetInfo],
            doc = "Features that should be enabled when this action is used.",
        ),
    },
    provides = [PwActionConfigSetInfo],
    doc = """Declares the configuration and selection of `pw_cc_tool` rules.

Action configs are bound to a toolchain through `action_configs`, and are the
driving mechanism for controlling toolchain tool invocation/behavior.

Action configs define three key things:

* Which tools to invoke for a given type of action.
* Tool features and compatibility.
* `pw_cc_flag_set`s that are unconditionally bound to a tool invocation.

Examples:

    pw_cc_action_config(
        name = "ar",
        action_names = ["@pw_toolchain//actions:all_ar_actions"],
        implies = [
            "@pw_toolchain//features/legacy:archiver_flags",
            "@pw_toolchain//features/legacy:linker_param_file",
        ],
        tools = [":ar_tool"],
    )

    pw_cc_action_config(
        name = "clang",
        action_names = [
            "@pw_toolchain//actions:all_asm_actions",
            "@pw_toolchain//actions:all_c_compiler_actions",
        ],
        tools = [":clang_tool"],
    )
""",
)
