# Copyright 2022 The Pigweed Authors
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
"""WORK IN PROGRESS!

# Overview of implementation

(If you just want to use the macros, see their docstrings; this section is
intended to orient future maintainers.)

Proto code generation is carried out by the _pwpb_proto_library,
_nanopb_proto_library, _pw_raw_rpc_proto_library and
_pw_nanopb_rpc_proto_library rules using aspects
(https://docs.bazel.build/versions/main/skylark/aspects.html).

As an example, _pwpb_proto_library has a single proto_library as a dependency,
but that proto_library may depend on other proto_library targets; as a result,
the generated .pwpb.h file #include's .pwpb.h files generated from the
dependency proto_libraries. The aspect propagates along the proto_library
dependency graph, running the proto compiler on each proto_library in the
original target's transitive dependencies, ensuring that we're not missing any
.pwpb.h files at C++ compile time.

Although we have a separate rule for each protocol compiler plugin
(_pwpb_proto_library, _nanopb_proto_library, _pw_raw_rpc_proto_library,
_pw_nanopb_rpc_proto_library), they actually share an implementation
(_impl_pw_proto_library) and use similar aspects, all generated by
_proto_compiler_aspect.
"""

load("@bazel_skylib//lib:paths.bzl", "paths")
load("@bazel_tools//tools/cpp:toolchain_utils.bzl", "find_cpp_toolchain", "use_cpp_toolchain")
load("@pigweed//pw_build/bazel_internal:pigweed_internal.bzl", "PW_DEFAULT_COPTS")
load("@rules_proto//proto:defs.bzl", "ProtoInfo")

# For Copybara use only
ADDITIONAL_PWPB_DEPS = []

def pwpb_proto_library(name, deps, tags = None, visibility = None):
    """A C++ proto library generated using pw_protobuf.

    Attributes:
      deps: proto_library targets for which to generate this library.
    """
    _pwpb_proto_library(
        name = name,
        protos = deps,
        deps = [
            Label("//pw_assert"),
            Label("//pw_containers:vector"),
            Label("//pw_preprocessor"),
            Label("//pw_protobuf"),
            Label("//pw_result"),
            Label("//pw_span"),
            Label("//pw_status"),
            Label("//pw_string:string"),
        ] + ADDITIONAL_PWPB_DEPS,
        tags = tags,
        visibility = visibility,
    )

def pwpb_rpc_proto_library(name, deps, pwpb_proto_library_deps, tags = None, visibility = None):
    """A pwpb_rpc proto library target.

    Attributes:
      deps: proto_library targets for which to generate this library.
      pwpb_proto_library_deps: A pwpb_proto_library generated
        from the same proto_library. Required.
    """
    _pw_pwpb_rpc_proto_library(
        name = name,
        protos = deps,
        deps = [
            Label("//pw_protobuf"),
            Label("//pw_rpc"),
            Label("//pw_rpc/pwpb:client_api"),
            Label("//pw_rpc/pwpb:server_api"),
        ] + pwpb_proto_library_deps,
        tags = tags,
        visibility = visibility,
    )

def raw_rpc_proto_library(name, deps, tags = None, visibility = None):
    """A raw C++ RPC proto library."""
    _pw_raw_rpc_proto_library(
        name = name,
        protos = deps,
        deps = [
            Label("//pw_rpc"),
            Label("//pw_rpc/raw:client_api"),
            Label("//pw_rpc/raw:server_api"),
        ],
        tags = tags,
        visibility = visibility,
    )

# TODO(b/234873954) Enable unused variable check.
# buildifier: disable=unused-variable
def nanopb_proto_library(name, deps, tags = [], visibility = None, options = None):
    """A C++ proto library generated using pw_protobuf.

    Attributes:
      deps: proto_library targets for which to generate this library.
    """

    # TODO(tpudlik): Find a way to get Nanopb to generate nested structs.
    # Otherwise add the manual tag to the resulting library, preventing it
    # from being built unless directly depended on.  e.g. The 'Pigweed'
    # message in
    # pw_protobuf/pw_protobuf_test_protos/full_test.proto will fail to
    # compile as it has a self referring nested message. According to
    # the docs
    # https://jpa.kapsi.fi/nanopb/docs/reference.html#proto-file-options
    # and https://github.com/nanopb/nanopb/issues/433 it seems like it
    # should be possible to configure nanopb to generate nested structs via
    # flags in .options files.
    #
    # One issue is nanopb doesn't silently ignore unknown options in .options
    # files so we can't share .options files with pwpb.
    extra_tags = ["manual"]
    _nanopb_proto_library(
        name = name,
        protos = deps,
        deps = [
            "@com_github_nanopb_nanopb//:nanopb",
            Label("//pw_assert"),
            Label("//pw_containers:vector"),
            Label("//pw_preprocessor"),
            Label("//pw_result"),
            Label("//pw_span"),
            Label("//pw_status"),
            Label("//pw_string:string"),
        ],
        tags = tags + extra_tags,
        visibility = visibility,
    )

def nanopb_rpc_proto_library(name, deps, nanopb_proto_library_deps, tags = [], visibility = None):
    """A C++ RPC proto library using nanopb.

    Attributes:
      deps: proto_library targets for which to generate this library.
      nanopb_proto_library_deps: A pw_nanopb_cc_library generated
        from the same proto_library. Required.
    """

    # See comment in nanopb_proto_library.
    extra_tags = ["manual"]
    _pw_nanopb_rpc_proto_library(
        name = name,
        protos = deps,
        deps = [
            Label("//pw_rpc"),
            Label("//pw_rpc/nanopb:client_api"),
            Label("//pw_rpc/nanopb:server_api"),
        ] + nanopb_proto_library_deps,
        tags = tags + extra_tags,
        visibility = visibility,
    )

def pw_proto_library(
        name,
        deps,
        visibility = None,
        tags = [],
        nanopb_options = None,
        enabled_targets = None):
    """Generate Pigweed proto C++ code.

    DEPRECATED. This macro is deprecated and will be removed in a future
    Pigweed version. Please use the single-target macros above.

    Args:
      name: The name of the target.
      deps: proto_library targets from which to generate Pigweed C++.
      visibility: The visibility of the target. See
         https://bazel.build/concepts/visibility.
      tags: Tags for the target. See
         https://bazel.build/reference/be/common-definitions#common-attributes.
      nanopb_options: path to file containing nanopb options, if any
        (https://jpa.kapsi.fi/nanopb/docs/reference.html#proto-file-options).
      enabled_targets: Specifies which libraries should be generated. Libraries
        will only be generated as needed, but unnecessary outputs may conflict
        with other build rules and thus cause build failures. This filter allows
        manual selection of which libraries should be supported by this build
        target in order to prevent such conflicts. The argument, if provided,
        should be a subset of ["pwpb", "nanopb", "raw_rpc", "nanopb_rpc"]. All
        are enabled by default. Note that "nanopb_rpc" relies on "nanopb".

    Example usage:

      proto_library(
        name = "benchmark_proto",
        srcs = [
          "benchmark.proto",
        ],
      )

      pw_proto_library(
        name = "benchmark_pw_proto",
        deps = [":benchmark_proto"],
      )

      pw_cc_binary(
        name = "proto_user",
        srcs = ["proto_user.cc"],
        deps = [":benchmark_pw_proto.pwpb"],
      )

    The pw_proto_library generates the following targets in this example:

    "benchmark_pw_proto.pwpb": C++ library exposing the "benchmark.pwpb.h" header.
    "benchmark_pw_proto.pwpb_rpc": C++ library exposing the
        "benchmark.rpc.pwpb.h" header.
    "benchmark_pw_proto.raw_rpc": C++ library exposing the "benchmark.raw_rpc.h"
        header.
    "benchmark_pw_proto.nanopb": C++ library exposing the "benchmark.pb.h"
        header.
    "benchmark_pw_proto.nanopb_rpc": C++ library exposing the
        "benchmark.rpc.pb.h" header.
    """

    def is_plugin_enabled(plugin):
        return (enabled_targets == None or plugin in enabled_targets)

    if is_plugin_enabled("nanopb"):
        # Use nanopb to generate the pb.h and pb.c files, and the target
        # exposing them.
        nanopb_proto_library(
            name = name + ".nanopb",
            deps = deps,
            tags = tags,
            visibility = visibility,
            options = nanopb_options,
        )

    if is_plugin_enabled("pwpb"):
        pwpb_proto_library(
            name = name + ".pwpb",
            deps = deps,
            tags = tags,
            visibility = visibility,
        )

    if is_plugin_enabled("pwpb_rpc"):
        pwpb_rpc_proto_library(
            name = name + ".pwpb_rpc",
            deps = deps,
            pwpb_proto_library_deps = [":" + name + ".pwpb"],
            tags = tags,
            visibility = visibility,
        )

    if is_plugin_enabled("raw_rpc"):
        raw_rpc_proto_library(
            name = name + ".raw_rpc",
            deps = deps,
            tags = tags,
            visibility = visibility,
        )

    if is_plugin_enabled("nanopb_rpc"):
        nanopb_rpc_proto_library(
            name = name + ".nanopb_rpc",
            deps = deps,
            nanopb_proto_library_deps = [":" + name + ".nanopb"],
            tags = tags,
            visibility = visibility,
        )

PwProtoInfo = provider(
    "Returned by PW proto compilation aspect",
    fields = {
        "hdrs": "generated C++ header files",
        "srcs": "generated C++ src files",
        "includes": "include paths for generated C++ header files",
    },
)

PwProtoOptionsInfo = provider(
    "Allows `pw_proto_filegroup` targets to pass along `.options` files " +
    "without polluting the `DefaultInfo` provider, which means they can " +
    "still be used in the `srcs` of `proto_library` targets.",
    fields = {
        "options_files": (".options file(s) associated with a proto_library " +
                          "for Pigweed codegen."),
    },
)

def _proto_compiler_aspect_impl(target, ctx):
    # List the files we will generate for this proto_library target.
    proto_info = target[ProtoInfo]

    srcs = []
    hdrs = []

    # Setup the output root for the plugin to point to targets output
    # directory. This allows us to declare the location of the files that protoc
    # will output in a way that `ctx.actions.declare_file` will understand,
    # since it works relative to the target.
    out_path = ctx.bin_dir.path
    if target.label.workspace_root:
        out_path += "/" + target.label.workspace_root
    if target.label.package:
        out_path += "/" + target.label.package

    # Add location of headers to cc include path.
    # Depending on prefix rules, the include path can be directly from the
    # output path, or underneath the package.
    includes = [out_path]

    for src in proto_info.direct_sources:
        # Get the relative import path for this .proto file.
        src_rel = paths.relativize(src.path, proto_info.proto_source_root)
        proto_dir = paths.dirname(src_rel)

        # Add location of headers to cc include path.
        includes.append("{}/{}".format(out_path, src.owner.package))

        for ext in ctx.attr._extensions:
            # Declare all output files, in target package dir.
            generated_filename = src.basename[:-len("proto")] + ext
            out_file = ctx.actions.declare_file("{}/{}".format(
                proto_dir,
                generated_filename,
            ))

            if ext.endswith(".h"):
                hdrs.append(out_file)
            else:
                srcs.append(out_file)

    # List the `.options` files from any `pw_proto_filegroup` targets listed
    # under this target's `srcs`.
    options_files = [
        options_file
        for src in ctx.rule.attr.srcs
        if PwProtoOptionsInfo in src
        for options_file in src[PwProtoOptionsInfo].options_files.to_list()
    ]

    # Local repository options files.
    options_file_include_paths = ["."]
    for options_file in options_files:
        # Handle .options files residing in external repositories.
        if options_file.owner.workspace_root:
            options_file_include_paths.append(options_file.owner.workspace_root)

        # Handle generated .options files.
        if options_file.root.path:
            options_file_include_paths.append(options_file.root.path)

    args = ctx.actions.args()
    for path in proto_info.transitive_proto_path.to_list():
        args.add("-I{}".format(path))

    args.add("--plugin=protoc-gen-custom={}".format(ctx.executable._protoc_plugin.path))

    # Convert include paths to a depset and back to deduplicate entries.
    for options_file_include_path in depset(options_file_include_paths).to_list():
        args.add("--custom_opt=-I{}".format(options_file_include_path))

    for plugin_option in ctx.attr._plugin_options:
        args.add("--custom_opt={}".format(plugin_option))

    args.add("--custom_out={}".format(out_path))
    args.add_all(proto_info.direct_sources)

    all_tools = [
        ctx.executable._protoc,
        ctx.executable._python_runtime,
        ctx.executable._protoc_plugin,
    ]
    run_path = [tool.dirname for tool in all_tools]

    ctx.actions.run(
        inputs = depset(
            direct = proto_info.direct_sources +
                     proto_info.transitive_sources.to_list() +
                     options_files,
            transitive = [proto_info.transitive_descriptor_sets],
        ),
        progress_message = "Generating %s C++ files for %s" % (ctx.attr._extensions, ctx.label.name),
        tools = all_tools,
        outputs = srcs + hdrs,
        executable = ctx.executable._protoc,
        arguments = [args],
        env = {
            "PATH": ":".join(run_path),

            # The nanopb protobuf plugin likes to compile some temporary protos
            # next to source files. This forces them to be written to Bazel's
            # genfiles directory.
            "NANOPB_PB2_TEMP_DIR": str(ctx.genfiles_dir),
        },
    )

    transitive_srcs = srcs
    transitive_hdrs = hdrs
    transitive_includes = includes
    for dep in ctx.rule.attr.deps:
        transitive_srcs += dep[PwProtoInfo].srcs
        transitive_hdrs += dep[PwProtoInfo].hdrs
        transitive_includes += dep[PwProtoInfo].includes
    return [PwProtoInfo(
        srcs = transitive_srcs,
        hdrs = transitive_hdrs,
        includes = transitive_includes,
    )]

def _proto_compiler_aspect(extensions, protoc_plugin, plugin_options = []):
    """Returns an aspect that runs the proto compiler.

    The aspect propagates through the deps of proto_library targets, running
    the proto compiler with the specified plugin for each of their source
    files. The proto compiler is assumed to produce one output file per input
    .proto file. That file is placed under bazel-bin at the same path as the
    input file, but with the specified extension (i.e., with _extensions = [
    .pwpb.h], the aspect converts pw_log/log.proto into
    bazel-bin/pw_log/log.pwpb.h).

    The aspect returns a provider exposing all the File objects generated from
    the dependency graph.
    """
    return aspect(
        attr_aspects = ["deps"],
        attrs = {
            "_extensions": attr.string_list(default = extensions),
            "_protoc": attr.label(
                default = Label("@com_google_protobuf//:protoc"),
                executable = True,
                cfg = "exec",
            ),
            "_protoc_plugin": attr.label(
                default = Label(protoc_plugin),
                executable = True,
                cfg = "exec",
            ),
            "_python_runtime": attr.label(
                default = Label("//:python3_interpreter"),
                allow_single_file = True,
                executable = True,
                cfg = "exec",
            ),
            "_plugin_options": attr.string_list(
                default = plugin_options,
            ),
        },
        implementation = _proto_compiler_aspect_impl,
        provides = [PwProtoInfo],
    )

def _compile_cc(
        ctx,
        srcs,
        hdrs,
        deps,
        includes = [],
        defines = [],
        user_compile_flags = []):
    """Compiles a list C++ source files.

    Returns:
      A CcInfo provider.
    """
    cc_toolchain = find_cpp_toolchain(ctx)
    feature_configuration = cc_common.configure_features(
        ctx = ctx,
        cc_toolchain = cc_toolchain,
        requested_features = ctx.features,
        unsupported_features = ctx.disabled_features,
    )

    compilation_contexts = [dep[CcInfo].compilation_context for dep in deps]
    compilation_context, compilation_outputs = cc_common.compile(
        name = ctx.label.name,
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        srcs = srcs,
        includes = includes,
        defines = defines,
        public_hdrs = hdrs,
        user_compile_flags = user_compile_flags,
        compilation_contexts = compilation_contexts,
    )

    linking_contexts = [dep[CcInfo].linking_context for dep in deps]
    linking_context, _ = cc_common.create_linking_context_from_compilation_outputs(
        actions = ctx.actions,
        feature_configuration = feature_configuration,
        cc_toolchain = cc_toolchain,
        compilation_outputs = compilation_outputs,
        linking_contexts = linking_contexts,
        disallow_dynamic_library = True,
        name = ctx.label.name,
    )

    transitive_output_files = [dep[DefaultInfo].files for dep in deps]
    output_files = depset(
        compilation_outputs.pic_objects + compilation_outputs.objects,
        transitive = transitive_output_files,
    )
    return [DefaultInfo(files = output_files), CcInfo(
        compilation_context = compilation_context,
        linking_context = linking_context,
    )]

def _impl_pw_proto_library(ctx):
    """Implementation of the proto codegen rule.

    The work of actually generating the code is done by the aspect, so here we
    compile and return a CcInfo to link against.
    """

    # Note that we don't distinguish between the files generated from the
    # target, and the files generated from its dependencies. We return all of
    # them together, and in pw_proto_library expose all of them as hdrs.
    # Pigweed's plugins happen to only generate .h files, so this works, but
    # strictly speaking we should expose only the files generated from the
    # target itself in hdrs, and place the headers generated from dependencies
    # in srcs. We don't perform layering_check in Pigweed, so this is not a big
    # deal.
    #
    # TODO(b/234873954): Tidy this up.
    all_srcs = []
    all_hdrs = []
    all_includes = []
    for dep in ctx.attr.protos:
        for f in dep[PwProtoInfo].hdrs:
            all_hdrs.append(f)
        for f in dep[PwProtoInfo].srcs:
            all_srcs.append(f)
        for i in dep[PwProtoInfo].includes:
            all_includes.append(i)

    return _compile_cc(
        ctx,
        all_srcs,
        all_hdrs,
        ctx.attr.deps,
        all_includes,
        defines = [],
        user_compile_flags = PW_DEFAULT_COPTS,
    )

# Instantiate the aspects and rules for generating code using specific plugins.
_pwpb_proto_compiler_aspect = _proto_compiler_aspect(
    ["pwpb.h"],
    "//pw_protobuf/py:plugin",
    ["--no-legacy-namespace"],
)

_pwpb_proto_library = rule(
    implementation = _impl_pw_proto_library,
    attrs = {
        "protos": attr.label_list(
            providers = [ProtoInfo],
            aspects = [_pwpb_proto_compiler_aspect],
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
)

_nanopb_proto_compiler_aspect = _proto_compiler_aspect(
    ["pb.h", "pb.c"],
    "@com_github_nanopb_nanopb//:protoc-gen-nanopb",
    ["--library-include-format=quote"],
)

_nanopb_proto_library = rule(
    implementation = _impl_pw_proto_library,
    attrs = {
        "protos": attr.label_list(
            providers = [ProtoInfo],
            aspects = [_nanopb_proto_compiler_aspect],
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
)

_pw_pwpb_rpc_proto_compiler_aspect = _proto_compiler_aspect(
    ["rpc.pwpb.h"],
    "//pw_rpc/py:plugin_pwpb",
    ["--no-legacy-namespace"],
)

_pw_pwpb_rpc_proto_library = rule(
    implementation = _impl_pw_proto_library,
    attrs = {
        "protos": attr.label_list(
            providers = [ProtoInfo],
            aspects = [_pw_pwpb_rpc_proto_compiler_aspect],
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
)

_pw_raw_rpc_proto_compiler_aspect = _proto_compiler_aspect(
    ["raw_rpc.pb.h"],
    "//pw_rpc/py:plugin_raw",
    ["--no-legacy-namespace"],
)

_pw_raw_rpc_proto_library = rule(
    implementation = _impl_pw_proto_library,
    attrs = {
        "protos": attr.label_list(
            providers = [ProtoInfo],
            aspects = [_pw_raw_rpc_proto_compiler_aspect],
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
)

_pw_nanopb_rpc_proto_compiler_aspect = _proto_compiler_aspect(
    ["rpc.pb.h"],
    "//pw_rpc/py:plugin_nanopb",
    ["--no-legacy-namespace"],
)

_pw_nanopb_rpc_proto_library = rule(
    implementation = _impl_pw_proto_library,
    attrs = {
        "protos": attr.label_list(
            providers = [ProtoInfo],
            aspects = [_pw_nanopb_rpc_proto_compiler_aspect],
        ),
        "deps": attr.label_list(
            providers = [CcInfo],
        ),
    },
    fragments = ["cpp"],
    toolchains = use_cpp_toolchain(),
)

def _pw_proto_filegroup_impl(ctx):
    source_files = list()
    options_files = list()

    for src in ctx.attr.srcs:
        source_files += src.files.to_list()

    for options_src in ctx.attr.options_files:
        for file in options_src.files.to_list():
            if file.extension == "options":
                options_files.append(file)
            else:
                fail((
                    "Files provided as `options_files` to a " +
                    "`pw_proto_filegroup` must have the `.options` " +
                    "extension; the file `{}` was provided."
                ).format(file.basename))

    return [
        DefaultInfo(files = depset(source_files)),
        PwProtoOptionsInfo(options_files = depset(options_files)),
    ]

pw_proto_filegroup = rule(
    doc = (
        "Acts like a `filegroup`, but with an additional `options_files` " +
        "attribute that accepts a list of `.options` files. These `.options` " +
        "files should typically correspond to `.proto` files provided under " +
        "the `srcs` attribute." +
        "\n\n" +
        "A `pw_proto_filegroup` is intended to be passed into the `srcs` of " +
        "a `proto_library` target as if it were a normal `filegroup` " +
        "containing only `.proto` files. For the purposes of the " +
        "`proto_library` itself, the `pw_proto_filegroup` does indeed act " +
        "just like a normal `filegroup`; the `options_files` attribute is " +
        "ignored. However, if that `proto_library` target is then passed " +
        "(directly or transitively) into the `deps` of a `pw_proto_library` " +
        "for code generation, the `pw_proto_library` target will have access " +
        "to the provided `.options` files and will pass them to the code " +
        "generator." +
        "\n\n" +
        "Note that, in order for a `pw_proto_filegroup` to be a valid `srcs` " +
        "entry for a `proto_library`, it must meet the same conditions " +
        "required of a standard `filegroup` in that context. Namely, its " +
        "`srcs` must provide at least one `.proto` (or `.protodevel`) file. " +
        "Put simply, a `pw_proto_filegroup` cannot be used as a vector for " +
        "injecting solely `.options` files; it must contain at least one " +
        "proto as well (generally one associated with an included `.options` " +
        "file in the interest of clarity)." +
        "\n\n" +
        "Regarding the somewhat unusual usage, this feature's design was " +
        "mostly preordained by the combination of Bazel's strict access " +
        "controls, the restrictions imposed on inputs to the `proto_library` " +
        "rule, and the need to support `.options` files from transitive " +
        "dependencies."
    ),
    implementation = _pw_proto_filegroup_impl,
    attrs = {
        "srcs": attr.label_list(
            allow_files = True,
        ),
        "options_files": attr.label_list(
            allow_files = True,
        ),
    },
    provides = [PwProtoOptionsInfo],
)
