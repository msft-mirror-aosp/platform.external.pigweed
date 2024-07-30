// Copyright 2024 The Pigweed Authors
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

/**
 * A file watcher for Bazel files.
 *
 * We use this to automatically trigger compile commands refreshes on changes
 * to the build graph.
 */

import * as vscode from 'vscode';

// Convert `exec` from callback style to promise style.
import { exec as cbExec } from 'child_process';
import util from 'node:util';
const exec = util.promisify(cbExec);

import { RefreshCallback, OK, refreshManager } from './refreshManager';
import logger from './logging';
import { getPigweedProjectRoot } from './project';
import { bazel_executable, settings, workingDir } from './settings';

/**
 * Create a container for a process running the refresh compile commands target.
 *
 * @return Refresh callbacks to do the refresh and to abort
 */
function createRefreshProcess(): [RefreshCallback, () => void] {
  // This provides us a handle to abort the process, if needed.
  const refreshController = new AbortController();
  const abort = () => refreshController.abort();
  const signal = refreshController.signal;

  // This callback will be registered with the RefreshManager to be called
  // when it's time to do the refresh.
  const cb: RefreshCallback = async () => {
    const cwd = (await getPigweedProjectRoot(
      settings,
      workingDir.get(),
    )) as string;

    logger.info('Refreshing compile commands');

    try {
      const { stdout, stderr } = await exec(
        // TODO: https://pwbug.dev/350861417 - This should use the Bazel
        // extension commands instead, but doing that through the VS Code
        // command API is not simple.
        `${bazel_executable.get()} run ${settings.refreshCompileCommandsTarget()}`,
        {
          cwd,
          signal,
        },
      );

      if (stdout.length > 0) {
        logger.info(stdout);
      }

      if (stderr.length > 0) {
        logger.info(stderr);
      }
    } catch (err: unknown) {
      const { message, code } = err as unknown as {
        message: string;
        code: string;
      };

      if (code === 'ABORT_ERR') {
        logger.info('Aborted refreshing compile commands');
        return OK;
      } else {
        logger.error(`Error refreshing compile commands: ${message}`);
        return { error: message };
      }
    }

    logger.info('Finished refreshing compile commands');
    return OK;
  };

  return [cb, abort];
}

/** Trigger a refresh compile commands process. */
export async function refreshCompileCommands() {
  const [cb, abort] = createRefreshProcess();

  const wrappedAbort = () => {
    abort();
    return OK;
  };

  refreshManager.onOnce(cb, 'refreshing');
  refreshManager.onOnce(wrappedAbort, 'abort');
  refreshManager.refresh();
}

/** Create file watchers to refresh compile commands on Bazel file changes. */
export function initRefreshCompileCommandsWatcher(): { dispose: () => void } {
  const watchers = [
    vscode.workspace.createFileSystemWatcher('**/BUILD.bazel'),
    vscode.workspace.createFileSystemWatcher('**/WORKSPACE'),
    vscode.workspace.createFileSystemWatcher('**/*.bzl'),
  ];

  watchers.forEach((watcher) => {
    watcher.onDidChange(refreshCompileCommands);
    watcher.onDidCreate(refreshCompileCommands);
    watcher.onDidDelete(refreshCompileCommands);
  });

  return {
    dispose: () => watchers.forEach((watcher) => watcher.dispose()),
  };
}
