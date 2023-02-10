#!/usr/bin/env python3
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
"""Tests the tokenized string decode module."""

from datetime import datetime
import math
import unittest

import tokenized_string_decoding_test_data as tokenized_string
import varint_test_data
from pw_tokenizer import decode
from pw_tokenizer import encode


def error(msg, value=None) -> str:
    """Formats msg as the message for an argument that failed to parse."""
    if value is None:
        return '<[{}]>'.format(msg)
    return '<[{} ({})]>'.format(msg, value)


class TestDecodeTokenized(unittest.TestCase):
    """Tests decoding tokenized strings with various arguments."""

    def test_decode_generated_data(self) -> None:
        self.assertGreater(len(tokenized_string.TEST_DATA), 100)

        for fmt, decoded, encoded in tokenized_string.TEST_DATA:
            self.assertEqual(decode.decode(fmt, encoded, True), decoded)

    def test_unicode_decode_errors(self) -> None:
        """Tests unicode errors, which do not occur in the C++ decoding code."""
        self.assertEqual(
            decode.decode('Why, %c', b'\x01', True),
            'Why, ' + error('%c ERROR', -1),
        )

        self.assertEqual(
            decode.decode('%sXY%+ldxy%u', b'\x83N\x80!\x01\x02', True),
            '{}XY{}xy{}'.format(
                error('%s ERROR', "'N\\x80!'"),
                error('%+ld SKIPPED', -1),
                error('%u SKIPPED', 1),
            ),
        )

        self.assertEqual(
            decode.decode('%s%lld%9u', b'\x82$\x80\x80', True),
            '{0}{1}{2}'.format(
                error("%s ERROR ('$\\x80')"),
                error('%lld SKIPPED'),
                error('%9u SKIPPED'),
            ),
        )

        self.assertEqual(
            decode.decode('%c', b'\xff\xff\xff\xff\x0f', True),
            error('%c ERROR', -2147483648),
        )

    def test_ignore_errors(self) -> None:
        self.assertEqual(decode.decode('Why, %c', b'\x01'), 'Why, %c')

        self.assertEqual(decode.decode('%s %d', b'\x01!'), '! %d')

    def test_pointer(self) -> None:
        """Tests pointer args, which are not natively supported in Python."""
        self.assertEqual(
            decode.decode('Hello: %p', b'\x00', True), 'Hello: 0x00000000'
        )
        self.assertEqual(
            decode.decode('%p%d%d', b'\x02\x80', True),
            '0x00000001<[%d ERROR]><[%d SKIPPED]>',
        )

    def test_nothing_printed_fails(self) -> None:
        result = decode.FormatString('%n').format(b'')
        self.assertFalse(result.ok())


class TestPercentLiteralDecoding(unittest.TestCase):
    """Tests decoding the %-literal in various invalid situations."""

    def test_percent(self):
        result = decode.FormatString('%%').format(b'')
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '%')
        self.assertEqual(result.remaining, b'')

    def test_percent_with_leading_plus_fails(self):
        result = decode.FormatString('%+%').format(b'')
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, b'')

    def test_percent_with_leading_negative(self):
        result = decode.FormatString('%-%').format(b'')
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, b'')

    def test_percent_with_leading_space(self):
        result = decode.FormatString('% %').format(b'')
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, b'')

    def test_percent_with_leading_hashtag(self):
        result = decode.FormatString('%#%').format(b'')
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, b'')

    def test_percent_with_leading_zero(self):
        result = decode.FormatString('%0%').format(b'')
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, b'')


# pylint: disable=too-many-public-methods
class TestIntegerDecoding(unittest.TestCase):
    """Tests decoding variable-length integers."""

    def test_signed_integer_i(self) -> None:
        result = decode.FormatString('%i').format(encode.encode_args(-10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '-10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_i_with_minus(self) -> None:
        result = decode.FormatString('%-5i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '10   ')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_i_with_plus(self) -> None:
        result = decode.FormatString('%+i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_i_with_blank_space(self) -> None:
        result = decode.FormatString('% i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' 10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_i_with_plus_and_blank_space_ignores_blank_space(
        self,
    ) -> None:
        result = decode.FormatString('%+ i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

        result = decode.FormatString('% +i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_i_with_hashtag(self) -> None:
        result = decode.FormatString('%#i').format(encode.encode_args(10))
        self.assertFalse(result.ok())

    def test_signed_integer_i_with_zero(self) -> None:
        result = decode.FormatString('%05i').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '00010')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d(self) -> None:
        result = decode.FormatString('%d').format(encode.encode_args(-10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '-10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d_with_minus(self) -> None:
        result = decode.FormatString('%-5d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '10   ')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d_with_plus(self) -> None:
        result = decode.FormatString('%+d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d_with_blank_space(self) -> None:
        result = decode.FormatString('% d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' 10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d_with_plus_and_blank_space_ignores_blank_space(
        self,
    ) -> None:
        result = decode.FormatString('%+ d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

        result = decode.FormatString('% +d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+10')
        self.assertEqual(result.remaining, b'')

    def test_signed_integer_d_with_hashtag(self) -> None:
        result = decode.FormatString('%#d').format(encode.encode_args(10))
        self.assertFalse(result.ok())

    def test_signed_integer_d_with_zero(self) -> None:
        result = decode.FormatString('%05d').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '00010')
        self.assertEqual(result.remaining, b'')

    def test_unsigned_integer(self) -> None:
        result = decode.FormatString('%u').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '10')
        self.assertEqual(result.remaining, b'')

    def test_unsigned_integer_with_hashtag(self) -> None:
        result = decode.FormatString('%#u').format(encode.encode_args(10))
        self.assertFalse(result.ok())

    def test_octal_integer(self) -> None:
        result = decode.FormatString('%o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '12')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_hashtag(self) -> None:
        result = decode.FormatString('%#o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_hashtag_and_width(self) -> None:
        result = decode.FormatString('%#10o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '       012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_hashtag_and_zero_and_width(self) -> None:
        result = decode.FormatString('%#010o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0000000012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_minus_and_hashtag(self) -> None:
        result = decode.FormatString('%#-5o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '012  ')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_plus_and_hashtag(self) -> None:
        result = decode.FormatString('%+#o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_space_and_hashtag(self) -> None:
        result = decode.FormatString('% #o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' 012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_zero_and_hashtag(self) -> None:
        result = decode.FormatString('%#05o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '00012')
        self.assertEqual(result.remaining, b'')

    def test_octal_integer_with_plus_and_space_and_hashtag_ignores_space(
        self,
    ) -> None:
        result = decode.FormatString('%+ #o').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+012')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_hex_integer(self) -> None:
        result = decode.FormatString('%x').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'a')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_hex_integer_with_hashtag(self) -> None:
        result = decode.FormatString('%#x').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0xa')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_hex_integer(self) -> None:
        result = decode.FormatString('%X').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'A')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_hex_integer_with_hashtag(self) -> None:
        result = decode.FormatString('%#X').format(encode.encode_args(10))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0XA')
        self.assertEqual(result.remaining, b'')

    def test_decode_generated_data(self) -> None:
        test_data = varint_test_data.TEST_DATA
        self.assertGreater(len(test_data), 100)

        for signed_spec, signed, unsigned_spec, unsigned, encoded in test_data:
            self.assertEqual(
                int(signed),
                decode.FormatSpec.from_string(signed_spec)
                .decode(bytearray(encoded))
                .value,
            )

            self.assertEqual(
                int(unsigned),
                decode.FormatSpec.from_string(unsigned_spec)
                .decode(bytearray(encoded))
                .value,
            )


class TestFloatDecoding(unittest.TestCase):
    """Tests decoding floating-point values."""

    def test_lowercase_float(self) -> None:
        result = decode.FormatString('%f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.200000')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_minus(self) -> None:
        result = decode.FormatString('%-10f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.200000  ')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_plus(self) -> None:
        result = decode.FormatString('%+f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+2.200000')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_blank_space(self) -> None:
        result = decode.FormatString('% f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' 2.200000')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_plus_and_blank_space_ignores_blank_space(
        self,
    ) -> None:
        result = decode.FormatString('%+ f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+2.200000')
        self.assertEqual(result.remaining, b'')

        result = decode.FormatString('% +f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+2.200000')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_hashtag(self) -> None:
        result = decode.FormatString('%.0f').format(encode.encode_args(2.0))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2')
        self.assertEqual(result.remaining, b'')

        result = decode.FormatString('%#.0f').format(encode.encode_args(2.0))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_with_zero(self) -> None:
        result = decode.FormatString('%010f').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '002.200000')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number(self) -> None:
        result = decode.FormatString('%f').format(encode.encode_args(math.inf))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'inf')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_with_minus(self) -> None:
        result = decode.FormatString('%-5f').format(
            encode.encode_args(math.inf)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'inf  ')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_with_plus(self) -> None:
        result = decode.FormatString('%+f').format(encode.encode_args(math.inf))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+inf')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_with_blank_space(self) -> None:
        result = decode.FormatString('% f').format(encode.encode_args(math.inf))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' inf')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_with_plus_and_blank_ignores_blank(
        self,
    ) -> None:
        result = decode.FormatString('%+ f').format(
            encode.encode_args(math.inf)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+inf')
        self.assertEqual(result.remaining, b'')

        result = decode.FormatString('% +f').format(
            encode.encode_args(math.inf)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+inf')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_with_hashtag(self) -> None:
        result = decode.FormatString('%#f').format(encode.encode_args(math.inf))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'inf')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_float_non_number_zero(self) -> None:
        result = decode.FormatString('%05f').format(
            encode.encode_args(math.inf)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '  inf')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_float(self) -> None:
        result = decode.FormatString('%F').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.200000')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_float_non_number(self) -> None:
        result = decode.FormatString('%F').format(encode.encode_args(math.inf))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'INF')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_exponential(self) -> None:
        result = decode.FormatString('%e').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.200000e+00')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_exponential(self) -> None:
        result = decode.FormatString('%E').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.200000E+00')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_shortest_take_normal(self) -> None:
        result = decode.FormatString('%g').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.2')
        self.assertEqual(result.remaining, b'')

    def test_lowercase_shortest_take_exponential(self) -> None:
        result = decode.FormatString('%g').format(encode.encode_args(1048580.0))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '1.04858e+06')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_shortest_take_normal(self) -> None:
        result = decode.FormatString('%G').format(encode.encode_args(2.2))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '2.2')
        self.assertEqual(result.remaining, b'')

    def test_uppercase_shortest_take_exponential(self) -> None:
        result = decode.FormatString('%G').format(encode.encode_args(1048580.0))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '1.04858E+06')
        self.assertEqual(result.remaining, b'')


class TestCharDecoding(unittest.TestCase):
    """Tests decoding character values."""

    def test_char(self) -> None:
        result = decode.FormatString('%c').format(encode.encode_args(ord('c')))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'c')
        self.assertEqual(result.remaining, b'')

    def test_char_with_minus(self) -> None:
        result = decode.FormatString('%-5c').format(
            encode.encode_args(ord('c'))
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'c    ')
        self.assertEqual(result.remaining, b'')

    def test_char_with_plus(self) -> None:
        result = decode.FormatString('%+c').format(encode.encode_args(ord('c')))
        self.assertFalse(result.ok())

    def test_char_with_blank_space(self) -> None:
        result = decode.FormatString('% c').format(encode.encode_args(ord('c')))
        self.assertFalse(result.ok())

    def test_char_with_hashtag(self) -> None:
        result = decode.FormatString('%#c').format(encode.encode_args(ord('c')))
        self.assertFalse(result.ok())

    def test_char_with_zero(self) -> None:
        result = decode.FormatString('%0c').format(encode.encode_args(ord('c')))
        self.assertFalse(result.ok())


class TestStringDecoding(unittest.TestCase):
    """Tests decoding string values."""

    def test_string(self) -> None:
        result = decode.FormatString('%s').format(encode.encode_args('hello'))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'hello')
        self.assertEqual(result.remaining, b'')

    def test_string_with_minus(self) -> None:
        result = decode.FormatString('%-6s').format(encode.encode_args('hello'))
        self.assertTrue(result.ok())
        self.assertEqual(result.value, 'hello ')
        self.assertEqual(result.remaining, b'')

    def test_string_with_plus(self) -> None:
        result = decode.FormatString('%+s').format(encode.encode_args('hello'))
        self.assertFalse(result.ok())

    def test_string_with_blank_space(self) -> None:
        result = decode.FormatString('% s').format(encode.encode_args('hello'))
        self.assertFalse(result.ok())

    def test_string_with_hashtag(self) -> None:
        result = decode.FormatString('%#s').format(encode.encode_args('hello'))
        self.assertFalse(result.ok())

    def test_string_with_zero(self) -> None:
        result = decode.FormatString('%0s').format(encode.encode_args('hello'))
        self.assertFalse(result.ok())


class TestPointerDecoding(unittest.TestCase):
    """Tests decoding pointer values."""

    def test_pointer(self) -> None:
        result = decode.FormatString('%p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0xDEADBEEF')
        self.assertEqual(result.remaining, b'')

    def test_pointer_0_padding(self) -> None:
        result = decode.FormatString('%p').format(
            encode.encode_args(0x00000000)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0x00000000')
        self.assertEqual(result.remaining, b'')

    def test_pointer_with_minus(self) -> None:
        result = decode.FormatString('%-12p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '0xDEADBEEF  ')
        self.assertEqual(result.remaining, b'')

    def test_pointer_with_plus(self) -> None:
        result = decode.FormatString('%+p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, '+0xDEADBEEF')
        self.assertEqual(result.remaining, b'')

    def test_pointer_with_blank_space(self) -> None:
        result = decode.FormatString('% p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertTrue(result.ok())
        self.assertEqual(result.value, ' 0xDEADBEEF')
        self.assertEqual(result.remaining, b'')

    def test_pointer_with_hashtag(self) -> None:
        result = decode.FormatString('%#p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, encode.encode_args(0xDEADBEEF))

    def test_pointer_with_zero(self) -> None:
        result = decode.FormatString('%0p').format(
            encode.encode_args(0xDEADBEEF)
        )
        self.assertFalse(result.ok())
        self.assertEqual(result.remaining, encode.encode_args(0xDEADBEEF))


class TestFormattedString(unittest.TestCase):
    """Tests scoring how successfully a formatted string decoded."""

    def test_no_args(self) -> None:
        result = decode.FormatString('string').format(b'')

        self.assertTrue(result.ok())
        self.assertEqual(result.score(), (True, True, 0, 0, datetime.max))

    def test_one_arg(self) -> None:
        result = decode.FormatString('%d').format(encode.encode_args(0))

        self.assertTrue(result.ok())
        self.assertEqual(result.score(), (True, True, 0, 1, datetime.max))

    def test_missing_args(self) -> None:
        result = decode.FormatString('%p%d%d').format(b'\x02\x80')

        self.assertFalse(result.ok())
        self.assertEqual(result.score(), (False, True, -2, 3, datetime.max))
        self.assertGreater(result.score(), result.score(datetime.now()))
        self.assertGreater(
            result.score(datetime.now()), result.score(datetime.min)
        )

    def test_compare_score(self) -> None:
        all_args_ok = decode.FormatString('%d%d%d').format(
            encode.encode_args(0, 0, 0)
        )
        missing_one_arg = decode.FormatString('%d%d%d').format(
            encode.encode_args(0, 0)
        )
        missing_two_args = decode.FormatString('%d%d%d').format(
            encode.encode_args(0)
        )
        all_args_extra_data = decode.FormatString('%d%d%d').format(
            encode.encode_args(0, 0, 0, 1)
        )
        missing_one_arg_extra_data = decode.FormatString('%d%d%d').format(
            b'\0' + b'\x80' * 100
        )

        self.assertGreater(all_args_ok.score(), missing_one_arg.score())
        self.assertGreater(missing_one_arg.score(), missing_two_args.score())
        self.assertGreater(
            missing_two_args.score(), all_args_extra_data.score()
        )
        self.assertGreater(
            all_args_extra_data.score(), missing_one_arg_extra_data.score()
        )


if __name__ == '__main__':
    unittest.main()
