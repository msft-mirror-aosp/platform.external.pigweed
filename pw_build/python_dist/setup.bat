:: Copyright 2021 The Pigweed Authors
::
:: Licensed under the Apache License, Version 2.0 (the "License"); you may not
:: use this file except in compliance with the License. You may obtain a copy of
:: the License at
::
::     https://www.apache.org/licenses/LICENSE-2.0
::
:: Unless required by applicable law or agreed to in writing, software
:: distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
:: WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
:: License for the specific language governing permissions and limitations under
:: the License.
@echo off

:: Check for the existence of a pip constraints file.
set constraints_arg=""
if exist %~dp0constraints.txt (
    set constraints_arg="-c %~dp0constraints.txt"
)

:: Generate python virtual environment using existing python.
python3 -m venv %~dp0python-venv

:: Install pip inside the virtual environment.
%~dp0python-venv\Scripts\python.exe -m pip install --upgrade pip

:: Install all wheel files.
for %%f in (%~dp0python_wheels\*) do %~dp0python-venv\Scripts\python.exe -m pip install --find-links=%~dp0python_wheels %constraints_arg% %%f
