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

#![no_main]
#![no_std]

// Panic handler that halts the CPU on panic.
use panic_halt as _;

// Cortex M runtime entry macro.
use cortex_m_rt::entry;

// Semihosting support which is well supported for QEMU targets.
use cortex_m_semihosting::{debug, hprintln};

#[entry]
fn main() -> ! {
    hprintln!("Hello, Pigweed!");
    debug::exit(debug::EXIT_SUCCESS);
    loop {}
}
