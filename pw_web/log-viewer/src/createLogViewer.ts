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

import { LogViewer as RootComponent } from './components/log-viewer';
import { StateStore, LocalStorageState } from './shared/state';
import { LogSourceEvent } from '../src/shared/interfaces';
import { LogSource } from '../src/log-source';
import { LogStore } from './log-store';

import '@material/web/button/filled-button.js';
import '@material/web/button/outlined-button.js';
import '@material/web/checkbox/checkbox.js';
import '@material/web/field/outlined-field.js';
import '@material/web/textfield/outlined-text-field.js';
import '@material/web/textfield/filled-text-field.js';
import '@material/web/icon/icon.js';
import '@material/web/iconbutton/icon-button.js';
import '@material/web/menu/menu.js';
import '@material/web/menu/menu-item.js';

/**
 * Create an instance of log-viewer
 * @param logSources - collection of sources from where logs originate
 * @param root - HTML component to append log-viewer to
 * @param state - handles state between sessions, defaults to localStorage
 * @param logStore - stores and handles management of all logs
 * @param columnOrder - defines column order between severity and message
 *   undefined fields are appended between defined order and message.
 */
export function createLogViewer(
  logSources: LogSource | LogSource[],
  root: HTMLElement,
  state: StateStore = new LocalStorageState(),
  logStore: LogStore = new LogStore(),
  columnOrder: string[] = ['log_source', 'time', 'timestamp'],
) {
  const logViewer = new RootComponent(state, columnOrder);
  root.appendChild(logViewer);
  let lastUpdateTimeoutId: NodeJS.Timeout;
  logStore.setColumnOrder(columnOrder);

  const logEntryListener = (event: LogSourceEvent) => {
    if (event.type === 'log-entry') {
      const logEntry = event.data;
      logStore.addLogEntry(logEntry);
      logViewer.logs = logStore.getLogs();
      if (lastUpdateTimeoutId) {
        clearTimeout(lastUpdateTimeoutId);
      }

      // Call requestUpdate at most once every 100 milliseconds.
      lastUpdateTimeoutId = setTimeout(() => {
        const updatedLogs = [...logStore.getLogs()];
        logViewer.logs = updatedLogs;
      }, 100);
    }
  };

  const sourcesArray = Array.isArray(logSources) ? logSources : [logSources];

  sourcesArray.forEach((logSource: LogSource) => {
    // Add the event listener to the LogSource instance
    logSource.addEventListener('log-entry', logEntryListener);
  });

  // Method to destroy and unsubscribe
  return () => {
    if (logViewer.parentNode) {
      logViewer.parentNode.removeChild(logViewer);
    }

    sourcesArray.forEach((logSource: LogSource) => {
      logSource.removeEventListener('log-entry', logEntryListener);
    });
  };
}
