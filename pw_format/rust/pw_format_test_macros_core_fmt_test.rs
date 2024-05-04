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

use pw_format::macros::IntegerDisplayType;

// Used to record calls into the test generator from `generator_test_macro!`.
#[derive(Debug, PartialEq)]
pub enum TestGeneratorOps {
    Finalize,
    StringFragment(String),
    IntegerConversion {
        display_type: IntegerDisplayType,
        type_width: u8,
        arg: String,
    },
    StringConversion(String),
    CharConversion(String),
    UntypedConversion(String),
}

// Used to record calls into the test generator from `printf_generator_test_macro!` and friends.
#[derive(Debug, PartialEq)]
pub enum PrintfTestGeneratorOps {
    Finalize,
    StringFragment(String),
    IntegerConversion { ty: String, arg: String },
    StringConversion(String),
    CharConversion(String),
    UntypedConversion(String),
}

#[cfg(test)]
mod tests {
    use pw_format_test_macros::{
        core_fmt_format_generator_test_macro, core_fmt_format_printf_generator_test_macro,
    };

    // Create an alias to ourselves so that the proc macro can name our crate.
    use crate as pw_format_test_macros_test;

    use super::*;

    #[test]
    fn generate_calls_generator_correctly() {
        assert_eq!(
            core_fmt_format_generator_test_macro!("test {}", 5),
            vec![
                TestGeneratorOps::StringFragment("test ".to_string()),
                TestGeneratorOps::UntypedConversion("5".to_string()),
                TestGeneratorOps::Finalize
            ]
        );
    }

    #[test]
    fn multiple_format_strings_are_concatenated() {
        assert_eq!(
            core_fmt_format_generator_test_macro!("a" PW_FMT_CONCAT "b"),
            vec![
                TestGeneratorOps::StringFragment("ab".to_string()),
                TestGeneratorOps::Finalize
            ]
        );
    }

    #[test]
    fn generate_printf_calls_generator_correctly() {
        // Currently only signed and unsigned integers are supported.  More
        // tests will be added as more types are supported with core::fmt style
        // format strings.
        assert_eq!(
            core_fmt_format_printf_generator_test_macro!("test {} {}", 5 as u32, -5 as i32),
            (
                "test %u %d",
                vec![
                    PrintfTestGeneratorOps::StringFragment("test ".to_string()),
                    PrintfTestGeneratorOps::UntypedConversion("5 as u32".to_string()),
                    PrintfTestGeneratorOps::StringFragment(" ".to_string()),
                    // Note here how `-5 as i32` gets converted to `- 5 as i32`
                    // during the conversion to tokens and back.
                    PrintfTestGeneratorOps::UntypedConversion("- 5 as i32".to_string()),
                    PrintfTestGeneratorOps::Finalize
                ]
            )
        );
    }
}
