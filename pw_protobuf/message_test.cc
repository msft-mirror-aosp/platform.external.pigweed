// Copyright 2021 The Pigweed Authors
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

#include "pw_protobuf/message.h"

#include "gtest/gtest.h"
#include "pw_stream/memory_stream.h"

#define ASSERT_OK(status) ASSERT_EQ(OkStatus(), status)

namespace pw::protobuf {

TEST(ProtoHelper, IterateMessage) {
  // clang-format off
  constexpr uint8_t encoded_proto[] = {
    // type=uint32, k=1, v=1
    0x08, 0x01,
    // type=uint32, k=2, v=2
    0x10, 0x02,
    // type=uint32, k=3, v=3
    0x18, 0x03,
  };
  // clang-format on

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  uint32_t count = 0;
  for (Message::Field field : parser) {
    ++count;
    EXPECT_EQ(field.field_number(), count);
    Uint32 value = field.As<Uint32>();
    ASSERT_OK(value.status());
    EXPECT_EQ(value.value(), count);
  }

  EXPECT_EQ(count, static_cast<uint32_t>(3));
}

TEST(ProtoHelper, MessageIterator) {
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // key = 1, str = "foo 1"
    0x0a, 0x05, 'f', 'o', 'o', ' ', '1',
    // type=uint32, k=2, v=2
    0x10, 0x02,
  };
  // clang-format on

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  Message::iterator iter = parser.begin();

  Message::iterator first = iter++;
  ASSERT_EQ(first, first);
  ASSERT_EQ(first->field_number(), static_cast<uint32_t>(1));
  String str = first->As<String>();
  ASSERT_OK(str.status());
  Result<bool> cmp = str.Equal("foo 1");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());

  Message::iterator second = iter++;
  ASSERT_EQ(second, second);
  ASSERT_EQ(second->field_number(), static_cast<uint32_t>(2));
  Uint32 uint32_val = second->As<Uint32>();
  ASSERT_OK(uint32_val.status());
  ASSERT_EQ(uint32_val.value(), static_cast<uint32_t>(2));

  ASSERT_NE(first, second);
  ASSERT_NE(first, iter);
  ASSERT_NE(second, iter);
  ASSERT_EQ(iter, parser.end());
}

TEST(ProtoHelper, AsProtoInteger) {
  // clang-format off
  std::uint8_t encoded_proto[] = {
      // type: int32, k = 1, val = -123
      0x08, 0x85, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x01,
      // type: uint32, k = 2, val = 123
      0x10, 0x7b,
      // type: sint32, k = 3, val = -456
      0x18, 0x8f, 0x07,
      // type: fixed32, k = 4, val = 268435457
      0x25, 0x01, 0x00, 0x00, 0x10,
      // type: sfixed32, k = 5, val = -268435457
      0x2d, 0xff, 0xff, 0xff, 0xef,
      // type: int64, k = 6, val = -1099511627776
      0x30, 0x80, 0x80, 0x80, 0x80, 0x80, 0xe0, 0xff, 0xff, 0xff, 0x01,
      // type: uint64, k = 7, val = 1099511627776
      0x38, 0x80, 0x80, 0x80, 0x80, 0x80, 0x20,
      // type: sint64, k = 8, val = -2199023255552
      0x40, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f,
      // type: fixed64, k = 9, val = 72057594037927937
      0x49, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01,
      // type: sfixed64, k = 10, val = -72057594037927937
      0x51, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe,
  };
  // clang-format on

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  {
    Int32 value = parser.AsInt32(1);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int32_t>(-123));
  }

  {
    Uint32 value = parser.AsUint32(2);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<uint32_t>(123));
  }

  {
    Sint32 value = parser.AsSint32(3);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int32_t>(-456));
  }

  {
    Fixed32 value = parser.AsFixed32(4);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<uint32_t>(268435457));
  }

  {
    Sfixed32 value = parser.AsSfixed32(5);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int32_t>(-268435457));
  }

  {
    Int64 value = parser.AsInt64(6);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int64_t>(-1099511627776));
  }

  {
    Uint64 value = parser.AsUint64(7);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<uint64_t>(1099511627776));
  }

  {
    Sint64 value = parser.AsSint64(8);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int64_t>(-2199023255552));
  }

  {
    Fixed64 value = parser.AsFixed64(9);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<uint64_t>(72057594037927937));
  }

  {
    Sfixed64 value = parser.AsSfixed64(10);
    ASSERT_OK(value.status());
    ASSERT_EQ(value.value(), static_cast<int64_t>(-72057594037927937));
  }
}

TEST(ProtoHelper, AsString) {
  // message {
  //   string str = 1;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // `str`, k = 1, "string"
    0x0a, 0x06, 's', 't', 'r', 'i', 'n', 'g',
  };
  // clang-format on

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  constexpr uint32_t kFieldNumber = 1;
  String value = parser.AsString(kFieldNumber);
  ASSERT_OK(value.status());
  Result<bool> cmp = value.Equal("string");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());

  cmp = value.Equal("other");
  ASSERT_OK(cmp.status());
  ASSERT_FALSE(cmp.value());

  // The string is a prefix of the target string to compare.
  cmp = value.Equal("string and more");
  ASSERT_OK(cmp.status());
  ASSERT_FALSE(cmp.value());

  // The target string to compare is a sub prefix of this string
  cmp = value.Equal("str");
  ASSERT_OK(cmp.status());
  ASSERT_FALSE(cmp.value());
}

TEST(ProtoHelper, AsRepeatedStrings) {
  // Repeated field of string i.e.
  //
  // message RepeatedString {
  //   repeated string msg_a = 1;
  //   repeated string msg_b = 2;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // key = 1, str = "foo 1"
    0x0a, 0x05, 'f', 'o', 'o', ' ', '1',
    // key = 2, str = "foo 2"
    0x12, 0x05, 'f', 'o', 'o', ' ', '2',
    // key = 1, str = "bar 1"
    0x0a, 0x05, 'b', 'a', 'r', ' ', '1',
    // key = 2, str = "bar 2"
    0x12, 0x05, 'b', 'a', 'r', ' ', '2',
  };
  // clang-format on

  constexpr uint32_t kMsgAFieldNumber = 1;
  constexpr uint32_t kMsgBFieldNumber = 2;
  constexpr uint32_t kNonExistFieldNumber = 3;

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  // Field 'msg_a'
  {
    RepeatedStrings msg = parser.AsRepeatedStrings(kMsgAFieldNumber);
    std::string_view expected[] = {
        "foo 1",
        "bar 1",
    };

    size_t count = 0;
    for (String ele : msg) {
      ASSERT_OK(ele.status());
      Result<bool> res = ele.Equal(expected[count++]);
      ASSERT_OK(res.status());
      ASSERT_TRUE(res.value());
    }

    ASSERT_EQ(count, static_cast<size_t>(2));
  }

  // Field `msg_b`
  {
    RepeatedStrings msg = parser.AsRepeatedStrings(kMsgBFieldNumber);
    std::string_view expected[] = {
        "foo 2",
        "bar 2",
    };

    size_t count = 0;
    for (String ele : msg) {
      ASSERT_OK(ele.status());
      Result<bool> res = ele.Equal(expected[count++]);
      ASSERT_OK(res.status());
      ASSERT_TRUE(res.value());
    }

    ASSERT_EQ(count, static_cast<size_t>(2));
  }

  // non-existing field
  {
    RepeatedStrings msg = parser.AsRepeatedStrings(kNonExistFieldNumber);
    size_t count = 0;
    for ([[maybe_unused]] String ele : msg) {
      count++;
    }

    ASSERT_EQ(count, static_cast<size_t>(0));
  }
}

TEST(ProtoHelper, RepeatedFieldIterator) {
  // Repeated field of string i.e.
  //
  // message RepeatedString {
  //   repeated string msg = 1;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // key = 1, str = "foo 1"
    0x0a, 0x05, 'f', 'o', 'o', ' ', '1',
    // key = 1, str = "bar 1"
    0x0a, 0x05, 'b', 'a', 'r', ' ', '1',
  };
  // clang-format on

  constexpr uint32_t kFieldNumber = 1;
  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));
  RepeatedStrings repeated_str = parser.AsRepeatedStrings(kFieldNumber);

  RepeatedStrings::iterator iter = repeated_str.begin();

  RepeatedStrings::iterator first = iter++;
  ASSERT_EQ(first, first);
  Result<bool> cmp = first->Equal("foo 1");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());

  RepeatedStrings::iterator second = iter++;
  ASSERT_EQ(second, second);
  cmp = second->Equal("bar 1");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());

  ASSERT_NE(first, second);
  ASSERT_NE(first, iter);
  ASSERT_NE(second, iter);
  ASSERT_EQ(iter, repeated_str.end());
}

TEST(ProtoHelper, AsMessage) {
  // A nested message:
  //
  // message Contact {
  //   string number = 1;
  //   string email = 2;
  // }
  //
  // message Person {
  //  Contact info = 2;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // Person.info.number = "123456", .email = "foo@email.com"
    0x12, 0x17,
    0x0a, 0x06, '1', '2', '3', '4', '5', '6',
    0x12, 0x0d, 'f', 'o', 'o', '@', 'e', 'm', 'a', 'i', 'l', '.', 'c', 'o', 'm',
  };
  // clang-format on

  constexpr uint32_t kInfoFieldNumber = 2;
  constexpr uint32_t kNumberFieldNumber = 1;
  constexpr uint32_t kEmailFieldNumber = 2;

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  Message info = parser.AsMessage(kInfoFieldNumber);
  ASSERT_OK(info.status());

  String number = info.AsString(kNumberFieldNumber);
  ASSERT_OK(number.status());
  Result<bool> cmp = number.Equal("123456");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());

  String email = info.AsString(kEmailFieldNumber);
  ASSERT_OK(email.status());
  cmp = email.Equal("foo@email.com");
  ASSERT_OK(cmp.status());
  ASSERT_TRUE(cmp.value());
}

TEST(ProtoHelper, AsRepeatedMessages) {
  // message Contact {
  //   string number = 1;
  //   string email = 2;
  // }
  //
  // message Person {
  //  repeated Contact info = 1;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // Person.Contact.number = "12345", .email = "foo@email.com"
    0x0a, 0x16,
    0x0a, 0x05, '1', '2', '3', '4', '5',
    0x12, 0x0d, 'f', 'o', 'o', '@', 'e', 'm', 'a', 'i', 'l', '.', 'c', 'o', 'm',

    // Person.Contact.number = "67890", .email = "bar@email.com"
    0x0a, 0x16,
    0x0a, 0x05, '6', '7', '8', '9', '0',
    0x12, 0x0d, 'b', 'a', 'r', '@', 'e', 'm', 'a', 'i', 'l', '.', 'c', 'o', 'm',
  };
  // clang-format on

  constexpr uint32_t kInfoFieldNumber = 1;
  constexpr uint32_t kNumberFieldNumber = 1;
  constexpr uint32_t kEmailFieldNumber = 2;

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  RepeatedMessages messages = parser.AsRepeatedMessages(kInfoFieldNumber);
  ASSERT_OK(messages.status());

  struct {
    std::string_view number;
    std::string_view email;
  } expected[] = {
      {"12345", "foo@email.com"},
      {"67890", "bar@email.com"},
  };

  size_t count = 0;
  for (Message message : messages) {
    String number = message.AsString(kNumberFieldNumber);
    ASSERT_OK(number.status());
    Result<bool> cmp = number.Equal(expected[count].number);
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    String email = message.AsString(kEmailFieldNumber);
    ASSERT_OK(email.status());
    cmp = email.Equal(expected[count].email);
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    count++;
  }

  ASSERT_EQ(count, static_cast<size_t>(2));
}

TEST(ProtoHelper, AsStringToBytesMap) {
  // message Maps {
  //   map<string, string> map_a = 1;
  //   map<string, string> map_b = 2;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // map_a["key_bar"] = "bar_a", key = 1
    0x0a, 0x10,
    0x0a, 0x07, 'k', 'e', 'y', '_', 'b', 'a', 'r', // map key
    0x12, 0x05, 'b', 'a', 'r', '_', 'a', // map value

    // map_a["key_foo"] = "foo_a", key = 1
    0x0a, 0x10,
    0x0a, 0x07, 'k', 'e', 'y', '_', 'f', 'o', 'o',
    0x12, 0x05, 'f', 'o', 'o', '_', 'a',

    // map_b["key_foo"] = "foo_b", key = 2
    0x12, 0x10,
    0x0a, 0x07, 'k', 'e', 'y', '_', 'f', 'o', 'o',
    0x12, 0x05, 'f', 'o', 'o', 0x5f, 0x62,

    // map_b["key_bar"] = "bar_b", key = 2
    0x12, 0x10,
    0x0a, 0x07, 'k', 'e', 'y', '_', 'b', 'a', 'r',
    0x12, 0x05, 'b', 'a', 'r', 0x5f, 0x62,
  };
  // clang-format on

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  {
    // Parse field 'map_a'
    constexpr uint32_t kFieldNumber = 1;
    StringMapParser<String> string_map =
        parser.AsStringToStringMap(kFieldNumber);

    String value = string_map["key_foo"];
    ASSERT_OK(value.status());
    Result<bool> cmp = value.Equal("foo_a");
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    value = string_map["key_bar"];
    ASSERT_OK(value.status());
    cmp = value.Equal("bar_a");
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    // Non-existing key
    value = string_map["non-existing"];
    ASSERT_EQ(value.status(), Status::NotFound());
  }

  {
    // Parse field 'map_b'
    constexpr uint32_t kFieldNumber = 2;
    StringMapParser<String> string_map =
        parser.AsStringToStringMap(kFieldNumber);

    String value = string_map["key_foo"];
    ASSERT_OK(value.status());
    Result<bool> cmp = value.Equal("foo_b");
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    value = string_map["key_bar"];
    ASSERT_OK(value.status());
    cmp = value.Equal("bar_b");
    ASSERT_OK(cmp.status());
    ASSERT_TRUE(cmp.value());

    // Non-existing key
    value = string_map["non-existing"];
    ASSERT_EQ(value.status(), Status::NotFound());
  }
}

TEST(ProtoHelper, AsStringToMessageMap) {
  // message Contact {
  //   string number = 1;
  //   string email = 2;
  // }
  //
  // message Contacts {
  //  map<string, Contact> staffs = 1;
  // }
  // clang-format off
  std::uint8_t encoded_proto[] = {
    // staffs['bar'] = {.number = '456, .email = "bar@email.com"}
    0x0a, 0x1b,
    0x0a, 0x03, 0x62, 0x61, 0x72,
    0x12, 0x14, 0x0a, 0x03, 0x34, 0x35, 0x36, 0x12, 0x0d, 0x62, 0x61, 0x72, 0x40, 0x65, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63, 0x6f, 0x6d,

    // staffs['foo'] = {.number = '123', .email = "foo@email.com"}
    0x0a, 0x1b,
    0x0a, 0x03, 0x66, 0x6f, 0x6f,
    0x12, 0x14, 0x0a, 0x03, 0x31, 0x32, 0x33, 0x12, 0x0d, 0x66, 0x6f, 0x6f, 0x40, 0x65, 0x6d, 0x61, 0x69, 0x6c, 0x2e, 0x63, 0x6f, 0x6d,
  };
  // clang-format on
  constexpr uint32_t kStaffsFieldId = 1;
  constexpr uint32_t kNumberFieldId = 1;
  constexpr uint32_t kEmailFieldId = 2;

  stream::MemoryReader reader(std::as_bytes(std::span(encoded_proto)));
  Message parser = Message(reader, sizeof(encoded_proto));

  StringMapParser<Message> staffs = parser.AsStringToMessageMap(kStaffsFieldId);
  ASSERT_OK(staffs.status());

  Message foo_staff = staffs["foo"];
  ASSERT_OK(foo_staff.status());
  String foo_number = foo_staff.AsString(kNumberFieldId);
  ASSERT_OK(foo_number.status());
  Result<bool> foo_number_cmp = foo_number.Equal("123");
  ASSERT_OK(foo_number_cmp.status());
  ASSERT_TRUE(foo_number_cmp.value());
  String foo_email = foo_staff.AsString(kEmailFieldId);
  ASSERT_OK(foo_email.status());
  Result<bool> foo_email_cmp = foo_email.Equal("foo@email.com");
  ASSERT_OK(foo_email_cmp.status());
  ASSERT_TRUE(foo_email_cmp.value());

  Message bar_staff = staffs["bar"];
  ASSERT_OK(bar_staff.status());
  String bar_number = bar_staff.AsString(kNumberFieldId);
  ASSERT_OK(bar_number.status());
  Result<bool> bar_number_cmp = bar_number.Equal("456");
  ASSERT_OK(bar_number_cmp.status());
  ASSERT_TRUE(bar_number_cmp.value());
  String bar_email = bar_staff.AsString(kEmailFieldId);
  ASSERT_OK(bar_email.status());
  Result<bool> bar_email_cmp = bar_email.Equal("bar@email.com");
  ASSERT_OK(bar_email_cmp.status());
  ASSERT_TRUE(bar_email_cmp.value());
}

}  // namespace pw::protobuf
