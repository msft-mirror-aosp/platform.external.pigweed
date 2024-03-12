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

import { LogSource } from '../log-source';
import { LogEntry, FieldData, Severity } from '../shared/interfaces';

import log_data from './log_data.json';

interface LevelToSeverity {
  [level: number]: Severity;
}

export class JsonLogSource extends LogSource {
  private intervalId: NodeJS.Timeout | null = null;
  private logIndex: number = 0;
  private previousLogTime: number = 0;

  private logLevelToSeverity: LevelToSeverity = {
    10: Severity.DEBUG,
    20: Severity.INFO,
    21: Severity.INFO,
    30: Severity.WARNING,
    40: Severity.ERROR,
    50: Severity.CRITICAL,
    70: Severity.CRITICAL,
  };

  private nonAdditionalDataFields = [
    '_hosttime',
    'levelname',
    'levelno',
    'args',
    'fields',
    'message',
    'time',
  ];

  constructor() {
    super();
  }

  start(): void {
    this.updateLogTime();

    const getInterval = () => {
      // Get the current log time
      const next_log_time = Number(log_data[this.logIndex].time);
      const wait_ms = 1000 * (next_log_time - this.previousLogTime);

      this.updateLogTime();
      return Math.round(wait_ms);
    };

    const readLogEntry = () => {
      const logEntry = this.readLogEntryFromJson();
      this.emitEvent('logEntry', logEntry);

      const nextInterval = getInterval();
      setTimeout(readLogEntry, nextInterval);
    };

    readLogEntry();
  }

  stop(): void {
    if (this.intervalId) {
      clearInterval(this.intervalId);
      this.intervalId = null;
    }
  }

  private updateLogTime(): void {
    this.previousLogTime = Number(log_data[this.logIndex].time);
  }

  private updateLogIndex(): void {
    this.logIndex += 1;
    if (this.logIndex >= log_data.length) {
      this.logIndex = 0;
    }
  }

  readLogEntryFromJson(): LogEntry {
    const data = log_data[this.logIndex];

    const host_log_time = new Date(0); // Date set to epoch seconds 0
    const host_log_epoch_seconds = Number(data.time);
    host_log_time.setUTCSeconds(Math.trunc(host_log_epoch_seconds));
    const host_log_epoch_milliseconds = Math.trunc(
      1000 * (host_log_epoch_seconds - Math.trunc(host_log_epoch_seconds)),
    );
    host_log_time.setUTCMilliseconds(host_log_epoch_milliseconds);

    const fields: Array<FieldData> = [
      { key: 'severity', value: this.logLevelToSeverity[data.levelno] },
      { key: 'time', value: host_log_time },
    ];

    Object.keys(data.fields).forEach((columnName) => {
      if (this.nonAdditionalDataFields.indexOf(columnName) === -1) {
        // @ts-ignore
        fields.push({ key: columnName, value: data.fields[columnName] });
      }
    });

    fields.push({ key: 'message', value: data.message });
    fields.push({ key: 'py_file', value: data.py_file || '' });
    fields.push({ key: 'py_logger', value: data.py_logger || '' });

    const logEntry: LogEntry = {
      severity: this.logLevelToSeverity[data.levelno],
      timestamp: new Date(),
      fields: fields,
    };

    this.updateLogIndex();

    return logEntry;
  }
}
