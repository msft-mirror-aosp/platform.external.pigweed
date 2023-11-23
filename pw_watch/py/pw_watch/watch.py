#!/usr/bin/env python
# Copyright 2020 The Pigweed Authors
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
"""Watch files for changes and rebuild.

Run arbitrary commands or invoke build systems (Ninja, Bazel and make) on one or
more build directories whenever source files change.

Examples:

  # Build the default target in out/ using ninja.
  pw watch -C out

  # Build python.lint and stm32f429i targets in out/ using ninja.
  pw watch python.lint stm32f429i

  # Build pw_run_tests.modules in the out/cmake directory
  pw watch -C out/cmake pw_run_tests.modules

  # Build the default target in out/ and pw_apps in out/cmake
  pw watch -C out -C out/cmake pw_apps

  # Build python.tests in out/ and pw_apps in out/cmake/
  pw watch python.tests -C out/cmake pw_apps

  # Run 'bazel build' and 'bazel test' on the target '//...' in outbazel/
  pw watch --run-command 'mkdir -p outbazel'
  -C outbazel '//...'
  --build-system-command outbazel 'bazel build'
  --build-system-command outbazel 'bazel test'
"""

import argparse
import concurrent.futures
import errno
import http.server
import logging
import os
from pathlib import Path
import re
import subprocess
import socketserver
import sys
import threading
from threading import Thread
from typing import (
    Callable,
    Iterable,
    List,
    NoReturn,
    Optional,
    Sequence,
    Tuple,
)

from watchdog.events import FileSystemEventHandler  # type: ignore[import]
from watchdog.observers import Observer  # type: ignore[import]

from prompt_toolkit import prompt

from pw_build.build_recipe import BuildRecipe, create_build_recipes
from pw_build.project_builder import (
    ProjectBuilder,
    execute_command_no_logging,
    execute_command_with_logging,
    log_build_recipe_start,
    log_build_recipe_finish,
    ASCII_CHARSET,
    EMOJI_CHARSET,
)
from pw_build.project_builder_context import get_project_builder_context
import pw_cli.branding
import pw_cli.color
import pw_cli.env
import pw_cli.log
import pw_cli.plugins
import pw_console.python_logging

from pw_watch.argparser import (
    WATCH_PATTERN_DELIMITER,
    WATCH_PATTERNS,
    add_parser_arguments,
)
from pw_watch.debounce import DebouncedFunction, Debouncer
from pw_watch.watch_app import WatchAppPrefs, WatchApp

_COLOR = pw_cli.color.colors()
_LOG = logging.getLogger('pw_build.watch')
_ERRNO_INOTIFY_LIMIT_REACHED = 28

# Suppress events under 'fsevents', generated by watchdog on every file
# event on MacOS.
# TODO: b/182281481 - Fix file ignoring, rather than just suppressing logs
_FSEVENTS_LOG = logging.getLogger('fsevents')
_FSEVENTS_LOG.setLevel(logging.WARNING)

_FULLSCREEN_STATUS_COLUMN_WIDTH = 10

BUILDER_CONTEXT = get_project_builder_context()


def git_ignored(file: Path) -> bool:
    """Returns true if this file is in a Git repo and ignored by that repo.

    Returns true for ignored files that were manually added to a repo.
    """
    file = file.resolve()
    directory = file.parent

    # Run the Git command from file's parent so that the correct repo is used.
    while True:
        try:
            returncode = subprocess.run(
                ['git', 'check-ignore', '--quiet', '--no-index', file],
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
                cwd=directory,
            ).returncode
            return returncode in (0, 128)
        except FileNotFoundError:
            # If the directory no longer exists, try parent directories until
            # an existing directory is found or all directories have been
            # checked. This approach makes it possible to check if a deleted
            # path is ignored in the repo it was originally created in.
            if directory == directory.parent:
                return False

            directory = directory.parent


class PigweedBuildWatcher(FileSystemEventHandler, DebouncedFunction):
    """Process filesystem events and launch builds if necessary."""

    # pylint: disable=too-many-instance-attributes
    NINJA_BUILD_STEP = re.compile(
        r'^\[(?P<step>[0-9]+)/(?P<total_steps>[0-9]+)\] (?P<action>.*)$'
    )
    _FILESYSTEM_EVENTS_THAT_TRIGGER_BUILDS = [
        'created',
        'modified',
        'deleted',
        'moved',
    ]

    def __init__(  # pylint: disable=too-many-arguments
        self,
        project_builder: ProjectBuilder,
        patterns: Sequence[str] = (),
        ignore_patterns: Sequence[str] = (),
        restart: bool = True,
        fullscreen: bool = False,
        banners: bool = True,
        use_logfile: bool = False,
        separate_logfiles: bool = False,
        parallel_workers: int = 1,
    ):
        super().__init__()

        self.banners = banners
        self.current_build_step = ''
        self.current_build_percent = 0.0
        self.current_build_errors = 0
        self.patterns = patterns
        self.ignore_patterns = ignore_patterns
        self.project_builder = project_builder
        self.parallel_workers = parallel_workers

        self.restart_on_changes = restart
        self.fullscreen_enabled = fullscreen
        self.watch_app: Optional[WatchApp] = None

        self.use_logfile = use_logfile
        self.separate_logfiles = separate_logfiles
        if self.parallel_workers > 1:
            self.separate_logfiles = True

        self.debouncer = Debouncer(self)

        # Track state of a build. These need to be members instead of locals
        # due to the split between dispatch(), run(), and on_complete().
        self.matching_path: Optional[Path] = None

        if (
            not self.fullscreen_enabled
            and not self.project_builder.should_use_progress_bars()
        ):
            self.wait_for_keypress_thread = threading.Thread(
                None, self._wait_for_enter
            )
            self.wait_for_keypress_thread.start()

        if self.fullscreen_enabled:
            BUILDER_CONTEXT.using_fullscreen = True

    def rebuild(self):
        """Rebuild command triggered from watch app."""
        self.debouncer.press('Manual build requested')

    def _wait_for_enter(self) -> None:
        try:
            while True:
                _ = prompt('')
                self.rebuild()
        # Ctrl-C on Unix generates KeyboardInterrupt
        # Ctrl-Z on Windows generates EOFError
        except (KeyboardInterrupt, EOFError):
            # Force stop any running ninja builds.
            _exit_due_to_interrupt()

    def _path_matches(self, path: Path) -> bool:
        """Returns true if path matches according to the watcher patterns"""
        return not any(path.match(x) for x in self.ignore_patterns) and any(
            path.match(x) for x in self.patterns
        )

    def dispatch(self, event) -> None:
        # There isn't any point in triggering builds on new directory creation.
        # It's the creation or modification of files that indicate something
        # meaningful enough changed for a build.
        if event.is_directory:
            return

        if event.event_type not in self._FILESYSTEM_EVENTS_THAT_TRIGGER_BUILDS:
            return

        # Collect paths of interest from the event.
        paths: List[str] = []
        if hasattr(event, 'dest_path'):
            paths.append(os.fsdecode(event.dest_path))
        if event.src_path:
            paths.append(os.fsdecode(event.src_path))
        for raw_path in paths:
            _LOG.debug('File event: %s', raw_path)

        # Check whether Git cares about any of these paths.
        for path in (Path(p).resolve() for p in paths):
            if not git_ignored(path) and self._path_matches(path):
                self._handle_matched_event(path)
                return

    def _handle_matched_event(self, matching_path: Path) -> None:
        if self.matching_path is None:
            self.matching_path = matching_path

        log_message = f'File change detected: {os.path.relpath(matching_path)}'
        if self.restart_on_changes:
            if self.fullscreen_enabled and self.watch_app:
                self.watch_app.clear_log_panes()
            self.debouncer.press(f'{log_message} Triggering build...')
        else:
            _LOG.info('%s ; not rebuilding', log_message)

    def _clear_screen(self) -> None:
        if self.fullscreen_enabled:
            return
        if self.project_builder.should_use_progress_bars():
            BUILDER_CONTEXT.clear_progress_scrollback()
            return
        print('\033c', end='')  # TODO(pwbug/38): Not Windows compatible.
        sys.stdout.flush()

    # Implementation of DebouncedFunction.run()
    #
    # Note: This will run on the timer thread created by the Debouncer, rather
    # than on the main thread that's watching file events. This enables the
    # watcher to continue receiving file change events during a build.
    def run(self) -> None:
        """Run all the builds and capture pass/fail for each."""

        # Clear the screen and show a banner indicating the build is starting.
        self._clear_screen()

        if self.banners:
            for line in pw_cli.branding.banner().splitlines():
                _LOG.info(line)
        if self.fullscreen_enabled:
            _LOG.info(
                self.project_builder.color.green(
                    'Watching for changes. Ctrl-d to exit; enter to rebuild'
                )
            )
        else:
            _LOG.info(
                self.project_builder.color.green(
                    'Watching for changes. Ctrl-C to exit; enter to rebuild'
                )
            )
        if self.matching_path:
            _LOG.info('')
            _LOG.info('Change detected: %s', self.matching_path)

        num_builds = len(self.project_builder)
        _LOG.info('Starting build with %d directories', num_builds)

        if self.project_builder.default_logfile:
            _LOG.info(
                '%s %s',
                self.project_builder.color.blue('Root logfile:'),
                self.project_builder.default_logfile.resolve(),
            )

        env = os.environ.copy()
        if self.project_builder.colors:
            # Force colors in Pigweed subcommands run through the watcher.
            env['PW_USE_COLOR'] = '1'
            # Force Ninja to output ANSI colors
            env['CLICOLOR_FORCE'] = '1'

        # Reset status
        BUILDER_CONTEXT.set_project_builder(self.project_builder)
        BUILDER_CONTEXT.set_enter_callback(self.rebuild)
        BUILDER_CONTEXT.set_building()

        for cfg in self.project_builder:
            cfg.reset_status()

        with concurrent.futures.ThreadPoolExecutor(
            max_workers=self.parallel_workers
        ) as executor:
            futures = []
            if (
                not self.fullscreen_enabled
                and self.project_builder.should_use_progress_bars()
            ):
                BUILDER_CONTEXT.add_progress_bars()

            for i, cfg in enumerate(self.project_builder, start=1):
                futures.append(executor.submit(self.run_recipe, i, cfg, env))

            for future in concurrent.futures.as_completed(futures):
                future.result()

        BUILDER_CONTEXT.set_idle()

    def run_recipe(self, index: int, cfg: BuildRecipe, env) -> None:
        if BUILDER_CONTEXT.interrupted():
            return
        if not cfg.enabled:
            return

        num_builds = len(self.project_builder)
        index_message = f'[{index}/{num_builds}]'

        log_build_recipe_start(
            index_message, self.project_builder, cfg, logger=_LOG
        )

        self.project_builder.run_build(
            cfg,
            env,
            index_message=index_message,
        )

        log_build_recipe_finish(
            index_message,
            self.project_builder,
            cfg,
            logger=_LOG,
        )

    def execute_command(
        self,
        command: list,
        env: dict,
        recipe: BuildRecipe,
        # pylint: disable=unused-argument
        *args,
        **kwargs,
        # pylint: enable=unused-argument
    ) -> bool:
        """Runs a command with a blank before/after for visual separation."""
        if self.fullscreen_enabled:
            return self._execute_command_watch_app(command, env, recipe)

        if self.separate_logfiles:
            return execute_command_with_logging(
                command, env, recipe, logger=recipe.log
            )

        if self.use_logfile:
            return execute_command_with_logging(
                command, env, recipe, logger=_LOG
            )

        return execute_command_no_logging(command, env, recipe)

    def _execute_command_watch_app(
        self,
        command: list,
        env: dict,
        recipe: BuildRecipe,
    ) -> bool:
        """Runs a command with and outputs the logs."""
        if not self.watch_app:
            return False

        self.watch_app.redraw_ui()

        def new_line_callback(recipe: BuildRecipe) -> None:
            self.current_build_step = recipe.status.current_step
            self.current_build_percent = recipe.status.percent
            self.current_build_errors = recipe.status.error_count

            if self.watch_app:
                self.watch_app.logs_redraw()

        desired_logger = _LOG
        if self.separate_logfiles:
            desired_logger = recipe.log

        result = execute_command_with_logging(
            command,
            env,
            recipe,
            logger=desired_logger,
            line_processed_callback=new_line_callback,
        )

        self.watch_app.redraw_ui()

        return result

    # Implementation of DebouncedFunction.cancel()
    def cancel(self) -> bool:
        if self.restart_on_changes:
            BUILDER_CONTEXT.restart_flag = True
            BUILDER_CONTEXT.terminate_and_wait()
            return True

        return False

    # Implementation of DebouncedFunction.on_complete()
    def on_complete(self, cancelled: bool = False) -> None:
        # First, use the standard logging facilities to report build status.
        if cancelled:
            _LOG.info('Build stopped.')
        elif BUILDER_CONTEXT.interrupted():
            pass  # Don't print anything.
        elif all(
            recipe.status.passed()
            for recipe in self.project_builder
            if recipe.enabled
        ):
            _LOG.info('Finished; all successful')
        else:
            _LOG.info('Finished; some builds failed')

        # For non-fullscreen pw watch
        if (
            not self.fullscreen_enabled
            and not self.project_builder.should_use_progress_bars()
        ):
            # Show a more distinct colored banner.
            self.project_builder.print_build_summary(
                cancelled=cancelled, logger=_LOG
            )
        self.project_builder.print_pass_fail_banner(
            cancelled=cancelled, logger=_LOG
        )

        if self.watch_app:
            self.watch_app.redraw_ui()
        self.matching_path = None

    # Implementation of DebouncedFunction.on_keyboard_interrupt()
    def on_keyboard_interrupt(self) -> None:
        _exit_due_to_interrupt()


def _exit(code: int) -> NoReturn:
    # Flush all log handlers
    logging.shutdown()
    # Note: The "proper" way to exit is via observer.stop(), then
    # running a join. However it's slower, so just exit immediately.
    #
    # Additionally, since there are several threads in the watcher, the usual
    # sys.exit approach doesn't work. Instead, run the low level exit which
    # kills all threads.
    os._exit(code)  # pylint: disable=protected-access


def _exit_due_to_interrupt() -> None:
    # To keep the log lines aligned with each other in the presence of
    # a '^C' from the keyboard interrupt, add a newline before the log.
    print('')
    _LOG.info('Got Ctrl-C; exiting...')
    BUILDER_CONTEXT.ctrl_c_interrupt()


def _log_inotify_watch_limit_reached():
    # Show information and suggested commands in OSError: inotify limit reached.
    _LOG.error(
        'Inotify watch limit reached: run this in your terminal if '
        'you are in Linux to temporarily increase inotify limit.'
    )
    _LOG.info('')
    _LOG.info(
        _COLOR.green(
            '        sudo sysctl fs.inotify.max_user_watches=' '$NEW_LIMIT$'
        )
    )
    _LOG.info('')
    _LOG.info(
        '  Change $NEW_LIMIT$ with an integer number, '
        'e.g., 20000 should be enough.'
    )


def _exit_due_to_inotify_watch_limit():
    _log_inotify_watch_limit_reached()
    _exit(1)


def _log_inotify_instance_limit_reached():
    # Show information and suggested commands in OSError: inotify limit reached.
    _LOG.error(
        'Inotify instance limit reached: run this in your terminal if '
        'you are in Linux to temporarily increase inotify limit.'
    )
    _LOG.info('')
    _LOG.info(
        _COLOR.green(
            '        sudo sysctl fs.inotify.max_user_instances=' '$NEW_LIMIT$'
        )
    )
    _LOG.info('')
    _LOG.info(
        '  Change $NEW_LIMIT$ with an integer number, '
        'e.g., 20000 should be enough.'
    )


def _exit_due_to_inotify_instance_limit():
    _log_inotify_instance_limit_reached()
    _exit(1)


def _exit_due_to_pigweed_not_installed():
    # Show information and suggested commands when pigweed environment variable
    # not found.
    _LOG.error(
        'Environment variable $PW_ROOT not defined or is defined '
        'outside the current directory.'
    )
    _LOG.error(
        'Did you forget to activate the Pigweed environment? '
        'Try source ./activate.sh'
    )
    _LOG.error(
        'Did you forget to install the Pigweed environment? '
        'Try source ./bootstrap.sh'
    )
    _exit(1)


# Go over each directory inside of the current directory.
# If it is not on the path of elements in directories_to_exclude, add
# (directory, True) to subdirectories_to_watch and later recursively call
# Observer() on them.
# Otherwise add (directory, False) to subdirectories_to_watch and later call
# Observer() with recursion=False.
def minimal_watch_directories(to_watch: Path, to_exclude: Iterable[Path]):
    """Determine which subdirectory to watch recursively"""
    try:
        to_watch = Path(to_watch)
    except TypeError:
        assert False, "Please watch one directory at a time."

    # Reformat to_exclude.
    directories_to_exclude: List[Path] = [
        to_watch.joinpath(directory_to_exclude)
        for directory_to_exclude in to_exclude
        if to_watch.joinpath(directory_to_exclude).is_dir()
    ]

    # Split the relative path of directories_to_exclude (compared to to_watch),
    # and generate all parent paths needed to be watched without recursion.
    exclude_dir_parents = {to_watch}
    for directory_to_exclude in directories_to_exclude:
        parts = list(Path(directory_to_exclude).relative_to(to_watch).parts)[
            :-1
        ]
        dir_tmp = to_watch
        for part in parts:
            dir_tmp = Path(dir_tmp, part)
            exclude_dir_parents.add(dir_tmp)

    # Go over all layers of directory. Append those that are the parents of
    # directories_to_exclude to the list with recursion==False, and others
    # with recursion==True.
    for directory in exclude_dir_parents:
        dir_path = Path(directory)
        yield dir_path, False
        for item in Path(directory).iterdir():
            if (
                item.is_dir()
                and item not in exclude_dir_parents
                and item not in directories_to_exclude
            ):
                yield item, True


def get_common_excludes() -> List[Path]:
    """Find commonly excluded directories, and return them as a [Path]"""
    exclude_list: List[Path] = []

    typical_ignored_directories: List[str] = [
        '.environment',  # Legacy bootstrap-created CIPD and Python venv.
        '.presubmit',  # Presubmit-created CIPD and Python venv.
        '.git',  # Pigweed's git repo.
        '.mypy_cache',  # Python static analyzer.
        '.cargo',  # Rust package manager.
        'environment',  # Bootstrap-created CIPD and Python venv.
        'out',  # Typical build directory.
    ]

    # Preset exclude list for Pigweed's upstream directories.
    pw_root_dir = Path(os.environ['PW_ROOT'])
    exclude_list.extend(
        pw_root_dir / ignored_directory
        for ignored_directory in typical_ignored_directories
    )

    # Preset exclude for common downstream project structures.
    #
    # If watch is invoked outside of the Pigweed root, exclude common
    # directories.
    pw_project_root_dir = Path(os.environ['PW_PROJECT_ROOT'])
    if pw_project_root_dir != pw_root_dir:
        exclude_list.extend(
            pw_project_root_dir / ignored_directory
            for ignored_directory in typical_ignored_directories
        )

    # Check for and warn about legacy directories.
    legacy_directories = [
        '.cipd',  # Legacy CIPD location.
        '.python3-venv',  # Legacy Python venv location.
    ]
    found_legacy = False
    for legacy_directory in legacy_directories:
        full_legacy_directory = pw_root_dir / legacy_directory
        if full_legacy_directory.is_dir():
            _LOG.warning(
                'Legacy environment directory found: %s',
                str(full_legacy_directory),
            )
            exclude_list.append(full_legacy_directory)
            found_legacy = True
    if found_legacy:
        _LOG.warning(
            'Found legacy environment directory(s); these ' 'should be deleted'
        )

    return exclude_list


def _simple_docs_server(
    address: str, port: int, path: Path
) -> Callable[[], None]:
    class Handler(http.server.SimpleHTTPRequestHandler):
        def __init__(self, *args, **kwargs):
            super().__init__(*args, directory=str(path), **kwargs)

        # Disable logs to stdout
        def log_message(
            self, format: str, *args  # pylint: disable=redefined-builtin
        ) -> None:
            return

    def simple_http_server_thread():
        with socketserver.TCPServer((address, port), Handler) as httpd:
            httpd.serve_forever()

    return simple_http_server_thread


def _serve_docs(
    build_dir: Path,
    docs_path: Path,
    address: str = '127.0.0.1',
    port: int = 8000,
) -> None:
    address = '127.0.0.1'
    docs_path = build_dir.joinpath(docs_path.joinpath('html'))
    server_thread = _simple_docs_server(address, port, docs_path)
    _LOG.info('Serving docs at http://%s:%d', address, port)

    # Spin up server in a new thread since it blocks
    threading.Thread(None, server_thread, 'pw_docs_server').start()


def watch_logging_init(log_level: int, fullscreen: bool, colors: bool) -> None:
    # Logging setup
    if not fullscreen:
        pw_cli.log.install(
            level=log_level,
            use_color=colors,
            hide_timestamp=False,
        )
        return

    watch_logfile = pw_console.python_logging.create_temp_log_file(
        prefix=__package__
    )

    pw_cli.log.install(
        level=logging.DEBUG,
        use_color=colors,
        hide_timestamp=False,
        log_file=watch_logfile,
    )


def watch_setup(  # pylint: disable=too-many-locals
    project_builder: ProjectBuilder,
    # NOTE: The following args should have defaults matching argparse. This
    # allows use of watch_setup by other project build scripts.
    patterns: str = WATCH_PATTERN_DELIMITER.join(WATCH_PATTERNS),
    ignore_patterns_string: str = '',
    exclude_list: Optional[List[Path]] = None,
    restart: bool = True,
    serve_docs: bool = False,
    serve_docs_port: int = 8000,
    serve_docs_path: Path = Path('docs/gen/docs'),
    fullscreen: bool = False,
    banners: bool = True,
    logfile: Optional[Path] = None,
    separate_logfiles: bool = False,
    parallel: bool = False,
    parallel_workers: int = 0,
    # pylint: disable=unused-argument
    default_build_targets: Optional[List[str]] = None,
    build_directories: Optional[List[str]] = None,
    build_system_commands: Optional[List[str]] = None,
    run_command: Optional[List[str]] = None,
    jobs: Optional[int] = None,
    keep_going: bool = False,
    colors: bool = True,
    debug_logging: bool = False,
    # pylint: enable=unused-argument
    # pylint: disable=too-many-arguments
) -> Tuple[PigweedBuildWatcher, List[Path]]:
    """Watches files and runs Ninja commands when they change."""
    watch_logging_init(
        log_level=project_builder.default_log_level,
        fullscreen=fullscreen,
        colors=colors,
    )

    # Update the project_builder log formatters since pw_cli.log.install may
    # have changed it.
    project_builder.apply_root_log_formatting()

    if project_builder.should_use_progress_bars():
        project_builder.use_stdout_proxy()

    _LOG.info('Starting Pigweed build watcher')

    # Get pigweed directory information from environment variable PW_ROOT.
    if os.environ['PW_ROOT'] is None:
        _exit_due_to_pigweed_not_installed()
    pw_root = Path(os.environ['PW_ROOT']).resolve()
    if Path.cwd().resolve() not in [pw_root, *pw_root.parents]:
        _exit_due_to_pigweed_not_installed()

    build_recipes = project_builder.build_recipes

    # Preset exclude list for pigweed directory.
    if not exclude_list:
        exclude_list = []
    exclude_list += get_common_excludes()
    # Add build directories to the exclude list.
    exclude_list.extend(
        cfg.build_dir.resolve()
        for cfg in build_recipes
        if isinstance(cfg.build_dir, Path)
    )

    for i, build_recipe in enumerate(build_recipes, start=1):
        _LOG.info('Will build [%d/%d]: %s', i, len(build_recipes), build_recipe)

    _LOG.debug('Patterns: %s', patterns)

    if serve_docs:
        _serve_docs(
            build_recipes[0].build_dir, serve_docs_path, port=serve_docs_port
        )

    # Ignore the user-specified patterns.
    ignore_patterns = (
        ignore_patterns_string.split(WATCH_PATTERN_DELIMITER)
        if ignore_patterns_string
        else []
    )

    # Add project_builder logfiles to ignore_patterns
    if project_builder.default_logfile:
        ignore_patterns.append(str(project_builder.default_logfile))
    if project_builder.separate_build_file_logging:
        for recipe in project_builder:
            if recipe.logfile:
                ignore_patterns.append(str(recipe.logfile))

    workers = 1
    if parallel:
        # If parallel is requested and parallel_workers is set to 0 run all
        # recipes in parallel. That is, use the number of recipes as the worker
        # count.
        if parallel_workers == 0:
            workers = len(project_builder)
        else:
            workers = parallel_workers

    event_handler = PigweedBuildWatcher(
        project_builder=project_builder,
        patterns=patterns.split(WATCH_PATTERN_DELIMITER),
        ignore_patterns=ignore_patterns,
        restart=restart,
        fullscreen=fullscreen,
        banners=banners,
        use_logfile=bool(logfile),
        separate_logfiles=separate_logfiles,
        parallel_workers=workers,
    )

    project_builder.execute_command = event_handler.execute_command

    return event_handler, exclude_list


def watch(
    event_handler: PigweedBuildWatcher,
    exclude_list: List[Path],
):
    """Watches files and runs Ninja commands when they change."""
    # Try to make a short display path for the watched directory that has
    # "$HOME" instead of the full home directory. This is nice for users
    # who have deeply nested home directories.
    path_to_log = str(Path().resolve()).replace(str(Path.home()), '$HOME')

    try:
        # It can take awhile to configure the filesystem watcher, so have the
        # message reflect that with the "...". Run inside the try: to
        # gracefully handle the user Ctrl-C'ing out during startup.

        _LOG.info('Attaching filesystem watcher to %s/...', path_to_log)

        # Observe changes for all files in the root directory. Whether the
        # directory should be observed recursively or not is determined by the
        # second element in subdirectories_to_watch.
        observers = []
        for path, rec in minimal_watch_directories(Path.cwd(), exclude_list):
            observer = Observer()
            observer.schedule(
                event_handler,
                str(path),
                recursive=rec,
            )
            observer.start()
            observers.append(observer)

        event_handler.debouncer.press('Triggering initial build...')
        for observer in observers:
            while observer.is_alive():
                observer.join(1)
        _LOG.error('observers joined')

    # Ctrl-C on Unix generates KeyboardInterrupt
    # Ctrl-Z on Windows generates EOFError
    except (KeyboardInterrupt, EOFError):
        _exit_due_to_interrupt()
    except OSError as err:
        if err.args[0] == _ERRNO_INOTIFY_LIMIT_REACHED:
            if event_handler.watch_app:
                event_handler.watch_app.exit(
                    log_after_shutdown=_log_inotify_watch_limit_reached
                )
            elif event_handler.project_builder.should_use_progress_bars():
                BUILDER_CONTEXT.exit(
                    log_after_shutdown=_log_inotify_watch_limit_reached,
                )
            else:
                _exit_due_to_inotify_watch_limit()
        if err.errno == errno.EMFILE:
            if event_handler.watch_app:
                event_handler.watch_app.exit(
                    log_after_shutdown=_log_inotify_instance_limit_reached
                )
            elif event_handler.project_builder.should_use_progress_bars():
                BUILDER_CONTEXT.exit(
                    log_after_shutdown=_log_inotify_instance_limit_reached
                )
            else:
                _exit_due_to_inotify_instance_limit()
        raise err


def run_watch(
    event_handler: PigweedBuildWatcher,
    exclude_list: List[Path],
    prefs: Optional[WatchAppPrefs] = None,
    fullscreen: bool = False,
) -> None:
    """Start pw_watch."""
    if not prefs:
        prefs = WatchAppPrefs(load_argparse_arguments=add_parser_arguments)

    if fullscreen:
        watch_thread = Thread(
            target=watch,
            args=(event_handler, exclude_list),
            daemon=True,
        )
        watch_thread.start()
        watch_app = WatchApp(
            event_handler=event_handler,
            prefs=prefs,
        )

        event_handler.watch_app = watch_app
        watch_app.run()

    else:
        watch(event_handler, exclude_list)


def get_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser = add_parser_arguments(parser)
    return parser


def main() -> None:
    """Watch files for changes and rebuild."""
    parser = get_parser()
    args = parser.parse_args()

    prefs = WatchAppPrefs(load_argparse_arguments=add_parser_arguments)
    prefs.apply_command_line_args(args)
    build_recipes = create_build_recipes(prefs)

    env = pw_cli.env.pigweed_environment()
    if env.PW_EMOJI:
        charset = EMOJI_CHARSET
    else:
        charset = ASCII_CHARSET

    # Force separate-logfiles for split window panes if running in parallel.
    separate_logfiles = args.separate_logfiles
    if args.parallel:
        separate_logfiles = True

    def _recipe_abort(*args) -> None:
        _LOG.critical(*args)

    project_builder = ProjectBuilder(
        build_recipes=build_recipes,
        jobs=args.jobs,
        banners=args.banners,
        keep_going=args.keep_going,
        colors=args.colors,
        charset=charset,
        separate_build_file_logging=separate_logfiles,
        root_logfile=args.logfile,
        root_logger=_LOG,
        log_level=logging.DEBUG if args.debug_logging else logging.INFO,
        abort_callback=_recipe_abort,
    )

    event_handler, exclude_list = watch_setup(project_builder, **vars(args))

    run_watch(
        event_handler,
        exclude_list,
        prefs=prefs,
        fullscreen=args.fullscreen,
    )


if __name__ == '__main__':
    main()
