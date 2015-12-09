////////////////////////////////////////////////////////////////////////////////
/// @brief Library to build up VPack documents.
///
/// DISCLAIMER
///
/// Copyright 2015 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Max Neunhoeffer
/// @author Jan Steemann
/// @author Copyright 2015, ArangoDB GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include <ostream>
#include <string>
#include <iostream>

#include "tests-common.h"

TEST(BuilderTest, AddObjectInArray) {
  Builder b;
  b.openArray();
  b.openObject();
  b.close();
  b.close();
  Slice s(b.slice());
  ASSERT_TRUE(s.isArray());
  ASSERT_EQ(1UL, s.length());
  Slice ss(s[0]);
  ASSERT_TRUE(ss.isObject());
  ASSERT_EQ(0UL, ss.length());
}

TEST(BuilderTest, CreateWithoutBufferOrOptions) {
  ASSERT_VELOCYPACK_EXCEPTION(new Builder(nullptr), Exception::InternalError);

  std::shared_ptr<Buffer<uint8_t>> buffer;
  ASSERT_VELOCYPACK_EXCEPTION(new Builder(buffer, nullptr),
                              Exception::InternalError);

  buffer.reset(new Buffer<uint8_t>());
  ASSERT_VELOCYPACK_EXCEPTION(new Builder(buffer, nullptr),
                              Exception::InternalError);

  Builder b;
  b.add(Value(123));
  Slice s = b.slice();

  ASSERT_VELOCYPACK_EXCEPTION(b.clone(s, nullptr), Exception::InternalError);
}

TEST(BuilderTest, Copy) {
  Builder b;
  b.openArray();
  for (int i = 0; i < 10; i++) {
    b.add(Value("abcdefghijklmnopqrstuvwxyz"));
  }
  b.close();

  Builder a(b);
  ASSERT_NE(a.buffer().get(), b.buffer().get());
  ASSERT_TRUE(a.buffer().get() != nullptr);
  ASSERT_TRUE(b.buffer().get() != nullptr);
}

TEST(BuilderTest, CopyWithoutOptions) {
  Builder b;
  b.options = nullptr;

  try {
    Builder a(b);
    ASSERT_FALSE(true);
  } catch (Exception const& ex) {
    ASSERT_EQ(Exception::InternalError, ex.errorCode());
  }
}

TEST(BuilderTest, CopyAssign) {
  Builder b;

  Builder a;
  a = b;

  ASSERT_NE(a.buffer().get(), b.buffer().get());
  ASSERT_TRUE(a.buffer().get() != nullptr);
  ASSERT_TRUE(b.buffer().get() != nullptr);
}

TEST(BuilderTest, CopyAssignWithoutOptions) {
  Builder b;
  b.options = nullptr;

  Builder a;
  try {
    a = b;
    ASSERT_FALSE(true);
  } catch (Exception const& ex) {
    ASSERT_EQ(Exception::InternalError, ex.errorCode());
  }
}

TEST(BuilderTest, Move) {
  Builder b;

  auto shptrb = b.buffer();
  Builder a(std::move(b));
  auto shptra = a.buffer();
  ASSERT_EQ(shptrb.get(), shptra.get());
  ASSERT_TRUE(a.buffer().get() != nullptr);
  ASSERT_TRUE(b.buffer().get() != nullptr);
}

TEST(BuilderTest, MoveWithoutOptions) {
  Builder b;
  b.options = nullptr;

  try {
    Builder a(std::move(b));
    ASSERT_FALSE(true);
  } catch (Exception const& ex) {
    ASSERT_EQ(Exception::InternalError, ex.errorCode());
  }
}

TEST(BuilderTest, MoveAssign) {
  Builder b;

  auto shptrb = b.buffer();
  Builder a = std::move(b);
  auto shptra = a.buffer();
  ASSERT_EQ(shptrb.get(), shptra.get());
  ASSERT_NE(a.buffer().get(), b.buffer().get());
  ASSERT_TRUE(a.buffer().get() != nullptr);
  ASSERT_TRUE(b.buffer().get() != nullptr);
}

TEST(BuilderTest, MoveAssignWithoutOptions) {
  Builder b;
  b.options = nullptr;

  Builder a;
  try {
    a = std::move(b);
    ASSERT_FALSE(true);
  } catch (Exception const& ex) {
    ASSERT_EQ(Exception::InternalError, ex.errorCode());
  }
}

TEST(BuilderTest, StealBuffer) {
  Builder b;
  b.openArray();
  for (int i = 0; i < 10; i++) {
    b.add(Value("abcdefghijklmnopqrstuvwxyz"));
  }
  b.close();

  auto ptr1 = b.buffer().get();
  std::shared_ptr<Buffer<uint8_t>> buf(b.steal());
  auto ptr2 = b.buffer().get();
  auto ptr3 = buf.get();
  ASSERT_EQ(ptr1, ptr3);
  ASSERT_NE(ptr2, ptr1);
}

TEST(BuilderTest, SizeWithOpenObject) {
  Builder b;
  ASSERT_EQ(0UL, b.size());

  b.add(Value(ValueType::Object));
  ASSERT_VELOCYPACK_EXCEPTION(b.size(), Exception::BuilderNotSealed);

  b.close();
  ASSERT_EQ(1UL, b.size());
}

TEST(BuilderTest, BufferSharedPointerNoSharing) {
  Builder b;
  b.add(Value(ValueType::Array));
  // construct a long string that will exceed the Builder's initial buffer
  b.add(Value(
      "skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddddddd"
      "dddddddjfkdfffffffffffffffffffffffff,"
      "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmmmm"
      "mmmmmmmmmmmmmmmmdddddddfjf"));
  b.close();

  std::shared_ptr<Buffer<uint8_t>> const& builderBuffer = b.buffer();

  // only the Builder itself is using the Buffer
  ASSERT_EQ(1, builderBuffer.use_count());
}

TEST(BuilderTest, BufferSharedPointerStealFromParser) {
  Parser parser;
  parser.parse(
      "\"skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddddd"
      "dddddddddjfkdfffffffffffffffffffffffff,"
      "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmmmm"
      "mmmmmmmmmmmmmmmmdddddddfjf\"");

  std::shared_ptr<Builder> b = parser.steal();
  // only the Builder itself is using its Buffer
  std::shared_ptr<Buffer<uint8_t>> const& builderBuffer = b->buffer();
  ASSERT_EQ(1, builderBuffer.use_count());
}

TEST(BuilderTest, BufferSharedPointerCopy) {
  Builder b;
  b.add(Value(ValueType::Array));
  // construct a long string that will exceed the Builder's initial buffer
  b.add(Value(
      "skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddddddd"
      "dddddddjfkdfffffffffffffffffffffffff,"
      "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmmmm"
      "mmmmmmmmmmmmmmmmdddddddfjf"));
  b.close();

  std::shared_ptr<Buffer<uint8_t>> const& builderBuffer = b.buffer();
  auto ptr = builderBuffer.get();

  // only the Builder itself is using its Buffer
  ASSERT_EQ(1, builderBuffer.use_count());

  std::shared_ptr<Buffer<uint8_t>> copy = b.buffer();
  ASSERT_EQ(2, copy.use_count());
  ASSERT_EQ(2, builderBuffer.use_count());

  copy.reset();
  ASSERT_EQ(1, builderBuffer.use_count());
  ASSERT_EQ(ptr, builderBuffer.get());
}

TEST(BuilderTest, BufferSharedPointerStealFromParserExitScope) {
  std::shared_ptr<Builder> b(new Builder());
  std::shared_ptr<Buffer<uint8_t>> builderBuffer = b->buffer();
  ASSERT_EQ(2, builderBuffer.use_count());
  auto ptr = builderBuffer.get();

  {
    Parser parser;
    parser.parse(
        "\"skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddd"
        "dddddddddddjfkdfffffffffffffffffffffffff,"
        "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmm"
        "mmmmmmmmmmmmmmmmmmdddddddfjf\"");

    ASSERT_EQ(2, builderBuffer.use_count());

    b = parser.steal();
    ASSERT_EQ(1, builderBuffer.use_count());
    std::shared_ptr<Buffer<uint8_t>> const& builderBuffer2 = b->buffer();
    ASSERT_NE(ptr, builderBuffer2.get());
    ASSERT_EQ(1, builderBuffer2.use_count());
  }

  ASSERT_EQ(1, builderBuffer.use_count());
  ASSERT_EQ(ptr, builderBuffer.get());
}

TEST(BuilderTest, BufferSharedPointerStealAndReturn) {
  auto func = []() -> std::shared_ptr<Builder> {
    Parser parser;
    parser.parse(
        "\"skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddd"
        "dddddddddddjfkdfffffffffffffffffffffffff,"
        "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmm"
        "mmmmmmmmmmmmmmmmmmdddddddfjf\"");

    return parser.steal();
  };

  std::shared_ptr<Builder> b = func();
  ASSERT_EQ(0xbf, *(b->buffer()->data()));  // long UTF-8 string...
  ASSERT_EQ(217UL, b->buffer()->size());
}

TEST(BuilderTest, BufferSharedPointerStealMultiple) {
  Parser parser;
  parser.parse(
      "\"skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddddd"
      "dddddddddjfkdfffffffffffffffffffffffff,"
      "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmmmm"
      "mmmmmmmmmmmmmmmmdddddddfjf\"");

  std::shared_ptr<Builder> b = parser.steal();
  ASSERT_EQ(0xbf, *(b->buffer()->data()));  // long UTF-8 string...
  ASSERT_EQ(217UL, b->buffer()->size());
  ASSERT_EQ(1, b->buffer().use_count());

  // steal again, should work, but Builder should be empty:
  std::shared_ptr<Builder> b2 = parser.steal();
  ASSERT_EQ(b2->buffer()->size(), 0UL);
}

TEST(BuilderTest, BufferSharedPointerInject) {
  std::shared_ptr<Buffer<uint8_t>> buffer(new Buffer<uint8_t>);
  auto ptr = buffer.get();

  Builder b(buffer);
  std::shared_ptr<Buffer<uint8_t>> const& builderBuffer = b.buffer();

  ASSERT_EQ(2, buffer.use_count());
  ASSERT_EQ(2, builderBuffer.use_count());
  ASSERT_EQ(ptr, builderBuffer.get());

  b.add(Value(ValueType::Array));
  // construct a long string that will exceed the Builder's initial buffer
  b.add(Value(
      "skjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjddddddddddddddddddddddddddddddddddd"
      "dddddddjfkdfffffffffffffffffffffffff,"
      "mmmmmmmmmmmmmmmmmmmmmmmmddddddddddddddddddddddddddddddddddddmmmmmmmmmmmm"
      "mmmmmmmmmmmmmmmmdddddddfjf"));
  b.close();

  std::shared_ptr<Buffer<uint8_t>> copy = b.buffer();
  ASSERT_EQ(3, buffer.use_count());
  ASSERT_EQ(3, copy.use_count());
  ASSERT_EQ(3, builderBuffer.use_count());
  ASSERT_EQ(ptr, copy.get());

  copy.reset();
  ASSERT_EQ(2, buffer.use_count());
  ASSERT_EQ(2, builderBuffer.use_count());

  b.steal();   // steals the buffer, resulting shared_ptr is forgotten
  ASSERT_EQ(1, buffer.use_count());
  ASSERT_EQ(ptr, buffer.get());
}

TEST(BuilderTest, AddNonCompoundTypeAllowUnindexed) {
  Builder b;
  b.add(Value(ValueType::Array, true));
  b.add(Value(ValueType::Object, true));

  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::None, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Null, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Bool, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Double, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::UTCDate, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::External, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::MinKey, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::MaxKey, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Int, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::UInt, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::SmallInt, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::String, true)),
                              Exception::InvalidValueType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Binary, true)),
                              Exception::InvalidValueType);
}

TEST(BuilderTest, BoolWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1.5, ValueType::Bool)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-100, ValueType::Bool)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<uint64_t>(100UL), ValueType::Bool)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::Bool)),
                              Exception::BuilderUnexpectedValue);
}

TEST(BuilderTest, StringWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1.5, ValueType::String)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-100, ValueType::String)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<uint64_t>(100UL), ValueType::String)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::String)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(false, ValueType::String)),
                              Exception::BuilderUnexpectedValue);
}

TEST(BuilderTest, SmallIntWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1.2, ValueType::SmallInt));
  b.add(Value(-1, ValueType::SmallInt));
  b.add(Value(static_cast<uint64_t>(1UL), ValueType::SmallInt));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-100, ValueType::SmallInt)),
                              Exception::NumberOutOfRange);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::SmallInt)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::SmallInt)),
                              Exception::BuilderUnexpectedValue);
  b.close();

  Slice s = b.slice();
  ASSERT_EQ("[1,-1,1]", s.toJson());
}

TEST(BuilderTest, IntWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1.2, ValueType::Int));
  b.add(Value(-1, ValueType::Int));
  b.add(Value(static_cast<uint64_t>(1UL), ValueType::Int));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::Int)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::Int)),
                              Exception::BuilderUnexpectedValue);
  b.close();

  Slice s = b.slice();
  ASSERT_EQ("[1,-1,1]", s.toJson());
}

TEST(BuilderTest, UIntWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1.2, ValueType::UInt));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-1.2, ValueType::UInt)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-1, ValueType::UInt)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::UInt)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::UInt)),
                              Exception::BuilderUnexpectedValue);
  b.add(Value(static_cast<uint64_t>(42UL), ValueType::UInt));
  b.add(Value(static_cast<int64_t>(23), ValueType::UInt));
  b.close();

  Slice s = b.slice();
  ASSERT_EQ("[1,42,23]", s.toJson());
}

TEST(BuilderTest, DoubleWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1.2, ValueType::Double));
  b.add(Value(-1.2, ValueType::Double));
  b.add(Value(-1, ValueType::Double));
  b.add(Value(static_cast<uint64_t>(1UL), ValueType::Double));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::Double)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::Double)),
                              Exception::BuilderUnexpectedValue);
  b.close();

  Slice s = b.slice();
  ASSERT_EQ("[1.2,-1.2,-1,1]", s.toJson());
}

TEST(BuilderTest, UTCDateWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1.2, ValueType::UTCDate));
  b.add(Value(static_cast<int64_t>(1), ValueType::UTCDate));
  b.add(Value(static_cast<uint64_t>(1UL), ValueType::UTCDate));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::UTCDate)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::UTCDate)),
                              Exception::BuilderUnexpectedValue);
}

TEST(BuilderTest, BinaryWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value("foobar", ValueType::Binary));
  b.add(Value(std::string("foobar"), ValueType::Binary));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1.5, ValueType::Binary)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<int64_t>(1), ValueType::Binary)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<uint64_t>(1), ValueType::Binary)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::Binary)),
                              Exception::BuilderUnexpectedValue);
}

TEST(BuilderTest, ExternalWithOtherTypes) {
  Builder b;
  b.add(Value(ValueType::Array));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value("foobar", ValueType::External)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(std::string("foobar"), ValueType::External)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1.5, ValueType::External)),
                              Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<int64_t>(1), ValueType::External)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(static_cast<uint64_t>(1), ValueType::External)),
      Exception::BuilderUnexpectedValue);
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true, ValueType::External)),
                              Exception::BuilderUnexpectedValue);
}

TEST(BuilderTest, AddAndOpenArray) {
  Builder b1;
  ASSERT_TRUE(b1.isClosed());
  b1.openArray();
  ASSERT_FALSE(b1.isClosed());
  b1.add(Value("bar"));
  b1.close();
  ASSERT_TRUE(b1.isClosed());
  ASSERT_EQ(0x02, b1.slice().head());

  Builder b2;
  ASSERT_TRUE(b2.isClosed());
  b2.openArray();
  ASSERT_FALSE(b2.isClosed());
  b2.add(Value("bar"));
  b2.close();
  ASSERT_TRUE(b2.isClosed());
  ASSERT_EQ(0x02, b2.slice().head());
}

TEST(BuilderTest, AddAndOpenObject) {
  Builder b1;
  ASSERT_TRUE(b1.isClosed());
  b1.openObject();
  ASSERT_FALSE(b1.isClosed());
  b1.add("foo", Value("bar"));
  b1.close();
  ASSERT_TRUE(b1.isClosed());
  ASSERT_EQ(0x0b, b1.slice().head());
  ASSERT_EQ("{\n  \"foo\" : \"bar\"\n}", b1.toString());
  ASSERT_EQ(1UL, b1.slice().length());

  Builder b2;
  ASSERT_TRUE(b2.isClosed());
  b2.openObject();
  ASSERT_FALSE(b2.isClosed());
  b2.add("foo", Value("bar"));
  b2.close();
  ASSERT_TRUE(b2.isClosed());
  ASSERT_EQ(0x0b, b2.slice().head());
  ASSERT_EQ("{\n  \"foo\" : \"bar\"\n}", b2.toString());
  ASSERT_EQ(1UL, b2.slice().length());
}

TEST(BuilderTest, MinKey) {
  Builder b;
  b.add(Value(ValueType::MinKey));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t const correctResult[] = {0x1e};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, MaxKey) {
  Builder b;
  b.add(Value(ValueType::MaxKey));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t const correctResult[] = {0x1f};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, Custom) {
  Builder b;
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Custom)),
                              Exception::BuilderUnexpectedType);
}

TEST(BuilderTest, None) {
  Builder b;
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::None)),
                              Exception::BuilderUnexpectedType);
}

TEST(BuilderTest, Null) {
  Builder b;
  b.add(Value(ValueType::Null));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t const correctResult[] = {0x18};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, False) {
  Builder b;
  b.add(Value(false));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t const correctResult[] = {0x19};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, True) {
  Builder b;
  b.add(Value(true));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t const correctResult[] = {0x1a};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, Int64) {
  static int64_t value = INT64_MAX;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[9] = {0x27, 0xff, 0xff, 0xff, 0xff,
                                     0xff, 0xff, 0xff, 0x7f};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, UInt64) {
  static uint64_t value = 1234;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[3] = {0x29, 0xd2, 0x04};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, Double) {
  static double value = 123.456;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[9] = {0x1b, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00};
  ASSERT_EQ(8ULL, sizeof(double));
  dumpDouble(value, correctResult + 1);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, String) {
  Builder b;
  b.add(Value("abcdefghijklmnopqrstuvwxyz"));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x5a, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66,
                                    0x67, 0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d,
                                    0x6e, 0x6f, 0x70, 0x71, 0x72, 0x73, 0x74,
                                    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArrayEmpty) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.close();
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x01};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArraySingleEntry) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(uint64_t(1)));
  b.close();
  uint8_t* result = b.start();
  ASSERT_EQ(0x02U, *result);
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x02, 0x03, 0x31};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArraySingleEntryLong) {
  std::string const value(
      "ngdddddljjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjsdddffffffffffffmmmmmmmmmmmmmmmsf"
      "dlllllllllllllllllllllllllllllllllllllllllllllllllrjjjjjjsdddddddddddddd"
      "ddddhhhhhhkkkkkkkksssssssssssssssssssssssssssssssssdddddddddddddddddkkkk"
      "kkkkkkkkksddddddddddddssssssssssfvvvvvvvvvvvvvvvvvvvvvvvvvvvfvgfff");
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(value));
  b.close();
  uint8_t* result = b.start();
  ASSERT_EQ(0x03U, *result);
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x03, 0x2c, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xbf, 0x1a, 0x01,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6e, 0x67, 0x64, 0x64, 0x64, 0x64,
      0x64, 0x6c, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a,
      0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a,
      0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x6a, 0x73, 0x64, 0x64,
      0x64, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x66,
      0x66, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d, 0x6d,
      0x6d, 0x6d, 0x6d, 0x6d, 0x73, 0x66, 0x64, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c,
      0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c,
      0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c,
      0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c,
      0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x6c, 0x72, 0x6a, 0x6a, 0x6a,
      0x6a, 0x6a, 0x6a, 0x73, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
      0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x68, 0x68,
      0x68, 0x68, 0x68, 0x68, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b,
      0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73,
      0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73,
      0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x64, 0x64, 0x64,
      0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
      0x64, 0x64, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b, 0x6b,
      0x6b, 0x6b, 0x6b, 0x73, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64, 0x64,
      0x64, 0x64, 0x64, 0x64, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73, 0x73,
      0x73, 0x73, 0x66, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76,
      0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x76,
      0x76, 0x76, 0x76, 0x76, 0x76, 0x76, 0x66, 0x76, 0x67, 0x66, 0x66, 0x66};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArraySameSizeEntries) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(uint64_t(1)));
  b.add(Value(uint64_t(2)));
  b.add(Value(uint64_t(3)));
  b.close();
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x02, 0x05, 0x31, 0x32, 0x33};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArraySomeValues) {
  double value = 2.3;
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(uint64_t(1200)));
  b.add(Value(value));
  b.add(Value("abc"));
  b.add(Value(true));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x06, 0x18, 0x04, 0x29, 0xb0, 0x04,  // uint(1200) = 0x4b0
      0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // double(2.3)
      0x43, 0x61, 0x62, 0x63, 0x1a, 0x03, 0x06, 0x0f, 0x13};
  dumpDouble(value, correctResult + 7);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ArrayCompact) {
  double value = 2.3;
  Builder b;
  b.add(Value(ValueType::Array, true));
  b.add(Value(uint64_t(1200)));
  b.add(Value(value));
  b.add(Value("abc"));
  b.add(Value(true));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x13, 0x14, 0x29, 0xb0, 0x04, 0x1b,
                                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                    0x00, 0x00,  // double
                                    0x43, 0x61, 0x62, 0x63, 0x1a, 0x04};
  dumpDouble(value, correctResult + 6);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectEmpty) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.close();
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x0a};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectEmptyCompact) {
  Builder b;
  b.add(Value(ValueType::Object, true));
  b.close();
  uint8_t* result = b.start();
  ValueLength len = b.size();

  // should still build the compact variant
  static uint8_t correctResult[] = {0x0a};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectSorted) {
  Options options;
  options.sortAttributeNames = true;

  double value = 2.3;
  Builder b(&options);
  b.add(Value(ValueType::Object));
  b.add("d", Value(uint64_t(1200)));
  b.add("c", Value(value));
  b.add("b", Value("abc"));
  b.add("a", Value(true));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x0b, 0x20, 0x04, 0x41, 0x64, 0x29, 0xb0, 0x04,  // "d": uint(1200) =
                                                       // 0x4b0
      0x41, 0x63, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // "c": double(2.3)
      0x41, 0x62, 0x43, 0x61, 0x62, 0x63,  // "b": "abc"
      0x41, 0x61, 0x1a,                    // "a": true
      0x19, 0x13, 0x08, 0x03};
  dumpDouble(value, correctResult + 11);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectUnsorted) {
  Options options;
  options.sortAttributeNames = false;

  double value = 2.3;
  Builder b(&options);
  b.add(Value(ValueType::Object));
  b.add("d", Value(uint64_t(1200)));
  b.add("c", Value(value));
  b.add("b", Value("abc"));
  b.add("a", Value(true));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x0f, 0x20, 0x04, 0x41, 0x64, 0x29, 0xb0, 0x04,  // "d": uint(1200) =
                                                       // 0x4b0
      0x41, 0x63, 0x1b, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      // "c": double(2.3)
      0x41, 0x62, 0x43, 0x61, 0x62, 0x63,  // "b": "abc"
      0x41, 0x61, 0x1a,                    // "a": true
      0x03, 0x08, 0x13, 0x19};
  dumpDouble(value, correctResult + 11);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectCompact) {
  double value = 2.3;
  Builder b;
  b.add(Value(ValueType::Object, true));
  b.add("d", Value(uint64_t(1200)));
  b.add("c", Value(value));
  b.add("b", Value("abc"));
  b.add("a", Value(true));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x14, 0x1c, 0x41, 0x64, 0x29, 0xb0, 0x04, 0x41, 0x63, 0x1b,
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,  // double
      0x41, 0x62, 0x43, 0x61, 0x62, 0x63, 0x41, 0x61, 0x1a, 0x04};

  dumpDouble(value, correctResult + 10);

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ObjectCompactBytesizeBelowThreshold) {
  Builder b;
  b.add(Value(ValueType::Array, true));
  for (size_t i = 0; i < 124; ++i) {
    b.add(Value(uint64_t(i % 10)));
  }
  b.close();

  uint8_t* result = b.start();
  Slice s(result);

  ASSERT_EQ(127UL, s.byteSize());

  ASSERT_EQ(0x13, result[0]);
  ASSERT_EQ(0x7f, result[1]);
  for (size_t i = 0; i < 124; ++i) {
    ASSERT_EQ(0x30 + (i % 10), result[2 + i]);
  }
  ASSERT_EQ(0x7c, result[126]);
}

TEST(BuilderTest, ObjectCompactBytesizeAboveThreshold) {
  Builder b;
  b.add(Value(ValueType::Array, true));
  for (size_t i = 0; i < 125; ++i) {
    b.add(Value(uint64_t(i % 10)));
  }
  b.close();

  uint8_t* result = b.start();
  Slice s(result);

  ASSERT_EQ(129UL, s.byteSize());

  ASSERT_EQ(0x13, result[0]);
  ASSERT_EQ(0x81, result[1]);
  ASSERT_EQ(0x01, result[2]);
  for (size_t i = 0; i < 125; ++i) {
    ASSERT_EQ(0x30 + (i % 10), result[3 + i]);
  }
  ASSERT_EQ(0x7d, result[128]);
}

TEST(BuilderTest, ObjectCompactLengthBelowThreshold) {
  Builder b;
  b.add(Value(ValueType::Array, true));
  for (size_t i = 0; i < 127; ++i) {
    b.add(Value("aaa"));
  }
  b.close();

  uint8_t* result = b.start();
  Slice s(result);

  ASSERT_EQ(512UL, s.byteSize());

  ASSERT_EQ(0x13, result[0]);
  ASSERT_EQ(0x80, result[1]);
  ASSERT_EQ(0x04, result[2]);
  for (size_t i = 0; i < 127; ++i) {
    ASSERT_EQ(0x43, result[3 + i * 4]);
  }
  ASSERT_EQ(0x7f, result[511]);
}

TEST(BuilderTest, ObjectCompactLengthAboveThreshold) {
  Builder b;
  b.add(Value(ValueType::Array, true));
  for (size_t i = 0; i < 128; ++i) {
    b.add(Value("aaa"));
  }
  b.close();

  uint8_t* result = b.start();
  Slice s(result);

  ASSERT_EQ(517UL, s.byteSize());

  ASSERT_EQ(0x13, result[0]);
  ASSERT_EQ(0x85, result[1]);
  ASSERT_EQ(0x04, result[2]);
  for (size_t i = 0; i < 128; ++i) {
    ASSERT_EQ(0x43, result[3 + i * 4]);
  }
  ASSERT_EQ(0x01, result[515]);
  ASSERT_EQ(0x80, result[516]);
}

TEST(BuilderTest, ExternalDisallowed) {
  uint8_t externalStuff[] = {0x01};
  Options options;
  options.disallowExternals = true;

  Builder b(&options);
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(Value(const_cast<void const*>(static_cast<void*>(externalStuff)),
                  ValueType::External)),
      Exception::BuilderExternalsDisallowed);
}

TEST(BuilderTest, External) {
  uint8_t externalStuff[] = {0x01};
  Builder b;
  b.add(Value(const_cast<void const*>(static_cast<void*>(externalStuff)),
              ValueType::External));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[1 + sizeof(char*)] = {0x00};
  correctResult[0] = 0x1d;
  uint8_t* p = externalStuff;
  memcpy(correctResult + 1, &p, sizeof(uint8_t*));

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ExternalUTCDate) {
  int64_t const v = -24549959465;
  Builder bExternal;
  bExternal.add(Value(v, ValueType::UTCDate));

  Builder b;
  b.add(Value(const_cast<void const*>(static_cast<void*>(bExternal.start()))));

  Slice s(b.start());
  ASSERT_EQ(ValueType::External, s.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, s.byteSize());
#else
  ASSERT_EQ(5ULL, s.byteSize());
#endif
  Slice sExternal(s.getExternal());
  ASSERT_EQ(9ULL, sExternal.byteSize());
  ASSERT_EQ(ValueType::UTCDate, sExternal.type());
  ASSERT_EQ(v, sExternal.getUTCDate());
}

TEST(BuilderTest, ExternalDouble) {
  double const v = -134.494401;
  Builder bExternal;
  bExternal.add(Value(v));

  Builder b;
  b.add(Value(const_cast<void const*>(static_cast<void*>(bExternal.start()))));

  Slice s(b.start());
  ASSERT_EQ(ValueType::External, s.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, s.byteSize());
#else
  ASSERT_EQ(5ULL, s.byteSize());
#endif

  Slice sExternal(s.getExternal());
  ASSERT_EQ(9ULL, sExternal.byteSize());
  ASSERT_EQ(ValueType::Double, sExternal.type());
  ASSERT_DOUBLE_EQ(v, sExternal.getDouble());
}

TEST(BuilderTest, ExternalBinary) {
  char const* p = "the quick brown FOX jumped over the lazy dog";
  Builder bExternal;
  bExternal.add(Value(std::string(p), ValueType::Binary));

  Builder b;
  b.add(Value(const_cast<void const*>(static_cast<void*>(bExternal.start()))));

  Slice s(b.start());
  ASSERT_EQ(ValueType::External, s.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, s.byteSize());
#else
  ASSERT_EQ(5ULL, s.byteSize());
#endif

  Slice sExternal(s.getExternal());
  ASSERT_EQ(2 + strlen(p), sExternal.byteSize());
  ASSERT_EQ(ValueType::Binary, sExternal.type());
  ValueLength len;
  uint8_t const* str = sExternal.getBinary(len);
  ASSERT_EQ(strlen(p), len);
  ASSERT_EQ(0, memcmp(str, p, len));
}

TEST(BuilderTest, ExternalString) {
  char const* p = "the quick brown FOX jumped over the lazy dog";
  Builder bExternal;
  bExternal.add(Value(std::string(p)));

  Builder b;
  b.add(Value(const_cast<void const*>(static_cast<void*>(bExternal.start()))));

  Slice s(b.start());
  ASSERT_EQ(ValueType::External, s.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, s.byteSize());
#else
  ASSERT_EQ(5ULL, s.byteSize());
#endif

  Slice sExternal(s.getExternal());
  ASSERT_EQ(1 + strlen(p), sExternal.byteSize());
  ASSERT_EQ(ValueType::String, sExternal.type());
  ValueLength len;
  char const* str = sExternal.getString(len);
  ASSERT_EQ(strlen(p), len);
  ASSERT_EQ(0, strncmp(str, p, len));
}

TEST(BuilderTest, ExternalExternal) {
  char const* p = "the quick brown FOX jumped over the lazy dog";
  Builder bExternal;
  bExternal.add(Value(std::string(p)));

  Builder bExExternal;
  bExExternal.add(
      Value(const_cast<void const*>(static_cast<void*>(bExternal.start()))));
  bExExternal.add(Value(std::string(p)));

  Builder b;
  b.add(
      Value(const_cast<void const*>(static_cast<void*>(bExExternal.start()))));

  Slice s(b.start());
  ASSERT_EQ(ValueType::External, s.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, s.byteSize());
#else
  ASSERT_EQ(5ULL, s.byteSize());
#endif

  Slice sExternal(s.getExternal());
  ASSERT_EQ(ValueType::External, sExternal.type());
#ifdef VELOCYPACK_64BIT
  ASSERT_EQ(9ULL, sExternal.byteSize());
#else
  ASSERT_EQ(5ULL, sExternal.byteSize());
#endif

  Slice sExExternal(sExternal.getExternal());
  ASSERT_EQ(1 + strlen(p), sExExternal.byteSize());
  ASSERT_EQ(ValueType::String, sExExternal.type());
  ValueLength len;
  char const* str = sExExternal.getString(len);
  ASSERT_EQ(strlen(p), len);
  ASSERT_EQ(0, strncmp(str, p, len));
}

TEST(BuilderTest, UInt) {
  uint64_t value = 0x12345678abcdef;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x2e, 0xef, 0xcd, 0xab,
                                    0x78, 0x56, 0x34, 0x12};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, IntPos) {
  int64_t value = 0x12345678abcdef;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x26, 0xef, 0xcd, 0xab,
                                    0x78, 0x56, 0x34, 0x12};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, IntNeg) {
  int64_t value = -0x12345678abcdef;
  Builder b;
  b.add(Value(value));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0x26, 0x11, 0x32, 0x54,
                                    0x87, 0xa9, 0xcb, 0xed};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, Int1Limits) {
  int64_t values[] = {-0x80LL,
                      0x7fLL,
                      -0x81LL,
                      0x80LL,
                      -0x8000LL,
                      0x7fffLL,
                      -0x8001LL,
                      0x8000LL,
                      -0x800000LL,
                      0x7fffffLL,
                      -0x800001LL,
                      0x800000LL,
                      -0x80000000LL,
                      0x7fffffffLL,
                      -0x80000001LL,
                      0x80000000LL,
                      -0x8000000000LL,
                      0x7fffffffffLL,
                      -0x8000000001LL,
                      0x8000000000LL,
                      -0x800000000000LL,
                      0x7fffffffffffLL,
                      -0x800000000001LL,
                      0x800000000000LL,
                      -0x80000000000000LL,
                      0x7fffffffffffffLL,
                      -0x80000000000001LL,
                      0x80000000000000LL,
                      arangodb::velocypack::toInt64(0x8000000000000000ULL),
                      0x7fffffffffffffffLL};
  for (size_t i = 0; i < sizeof(values) / sizeof(int64_t); i++) {
    int64_t v = values[i];
    Builder b;
    b.add(Value(v));
    uint8_t* result = b.start();
    Slice s(result);
    ASSERT_TRUE(s.isInt());
    ASSERT_EQ(v, s.getInt());
  }
}

TEST(BuilderTest, StringChar) {
  char const* value = "der fuxx ging in den wald und aß pilze";
  size_t const valueLen = strlen(value);
  Builder b;
  b.add(Value(value));

  Slice slice = Slice(b.start());
  ASSERT_TRUE(slice.isString());

  ValueLength len;
  char const* s = slice.getString(len);
  ASSERT_EQ(valueLen, len);
  ASSERT_EQ(0, strncmp(s, value, valueLen));

  std::string c = slice.copyString();
  ASSERT_EQ(valueLen, c.size());
  ASSERT_EQ(0, strncmp(value, c.c_str(), valueLen));
}

TEST(BuilderTest, StringString) {
  std::string const value("der fuxx ging in den wald und aß pilze");
  Builder b;
  b.add(Value(value));

  Slice slice = Slice(b.start());
  ASSERT_TRUE(slice.isString());

  ValueLength len;
  char const* s = slice.getString(len);
  ASSERT_EQ(value.size(), len);
  ASSERT_EQ(0, strncmp(s, value.c_str(), value.size()));

  std::string c = slice.copyString();
  ASSERT_EQ(value.size(), c.size());
  ASSERT_EQ(value, c);
}

TEST(BuilderTest, BinaryViaValuePair) {
  uint8_t binaryStuff[] = {0x02, 0x03, 0x05, 0x08, 0x0d};

  Builder b;
  b.add(ValuePair(binaryStuff, sizeof(binaryStuff)));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {0xc0, 0x05, 0x02, 0x03, 0x05, 0x08, 0x0d};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, ShortStringViaValuePair) {
  char const* p = "the quick brown fox jumped over the lazy dog";

  Builder b;
  uint8_t* dst = b.add(ValuePair(p, strlen(p), ValueType::String));
  memcpy(dst, p, strlen(p));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x6c, 0x74, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6b, 0x20, 0x62,
      0x72, 0x6f, 0x77, 0x6e, 0x20, 0x66, 0x6f, 0x78, 0x20, 0x6a, 0x75, 0x6d,
      0x70, 0x65, 0x64, 0x20, 0x6f, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65,
      0x20, 0x6c, 0x61, 0x7a, 0x79, 0x20, 0x64, 0x6f, 0x67};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, LongStringViaValuePair) {
  char const* p =
      "the quick brown fox jumped over the lazy dog, and it jumped and jumped "
      "and jumped and went on. But then, the String needed to get even longer "
      "and longer until the test finally worked.";

  Builder b;
  uint8_t* dst = b.add(ValuePair(p, strlen(p), ValueType::String));
  memcpy(dst, p, strlen(p));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0xbf, 0xb7, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x74, 0x68, 0x65,
      0x20, 0x71, 0x75, 0x69, 0x63, 0x6b, 0x20, 0x62, 0x72, 0x6f, 0x77, 0x6e,
      0x20, 0x66, 0x6f, 0x78, 0x20, 0x6a, 0x75, 0x6d, 0x70, 0x65, 0x64, 0x20,
      0x6f, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65, 0x20, 0x6c, 0x61, 0x7a,
      0x79, 0x20, 0x64, 0x6f, 0x67, 0x2c, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x69,
      0x74, 0x20, 0x6a, 0x75, 0x6d, 0x70, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64,
      0x20, 0x6a, 0x75, 0x6d, 0x70, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20,
      0x6a, 0x75, 0x6d, 0x70, 0x65, 0x64, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x77,
      0x65, 0x6e, 0x74, 0x20, 0x6f, 0x6e, 0x2e, 0x20, 0x42, 0x75, 0x74, 0x20,
      0x74, 0x68, 0x65, 0x6e, 0x2c, 0x20, 0x74, 0x68, 0x65, 0x20, 0x53, 0x74,
      0x72, 0x69, 0x6e, 0x67, 0x20, 0x6e, 0x65, 0x65, 0x64, 0x65, 0x64, 0x20,
      0x74, 0x6f, 0x20, 0x67, 0x65, 0x74, 0x20, 0x65, 0x76, 0x65, 0x6e, 0x20,
      0x6c, 0x6f, 0x6e, 0x67, 0x65, 0x72, 0x20, 0x61, 0x6e, 0x64, 0x20, 0x6c,
      0x6f, 0x6e, 0x67, 0x65, 0x72, 0x20, 0x75, 0x6e, 0x74, 0x69, 0x6c, 0x20,
      0x74, 0x68, 0x65, 0x20, 0x74, 0x65, 0x73, 0x74, 0x20, 0x66, 0x69, 0x6e,
      0x61, 0x6c, 0x6c, 0x79, 0x20, 0x77, 0x6f, 0x72, 0x6b, 0x65, 0x64, 0x2e};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, CustomViaValuePair) {
  char const* p = "\xf0the quick brown fox jumped over the lazy dog";

  Builder b;
  b.add(ValuePair(p, strlen(p), ValueType::Custom));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0xf0, 0x74, 0x68, 0x65, 0x20, 0x71, 0x75, 0x69, 0x63, 0x6b, 0x20, 0x62,
      0x72, 0x6f, 0x77, 0x6e, 0x20, 0x66, 0x6f, 0x78, 0x20, 0x6a, 0x75, 0x6d,
      0x70, 0x65, 0x64, 0x20, 0x6f, 0x76, 0x65, 0x72, 0x20, 0x74, 0x68, 0x65,
      0x20, 0x6c, 0x61, 0x7a, 0x79, 0x20, 0x64, 0x6f, 0x67};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, InvalidTypeViaValuePair) {
  char const* p = "fail";

  Builder b;
  ASSERT_VELOCYPACK_EXCEPTION(
      b.add(ValuePair(p, strlen(p), ValueType::UTCDate)),
      Exception::BuilderUnexpectedType);
}

TEST(BuilderTest, UTCDate) {
  int64_t const value = 12345678;
  Builder b;
  b.add(Value(value, ValueType::UTCDate));

  Slice s(b.start());
  ASSERT_EQ(0x1cU, s.head());
  ASSERT_TRUE(s.isUTCDate());
  ASSERT_EQ(9UL, s.byteSize());
  ASSERT_EQ(value, s.getUTCDate());
}

TEST(BuilderTest, UTCDateZero) {
  int64_t const value = 0;
  Builder b;
  b.add(Value(value, ValueType::UTCDate));

  Slice s(b.start());
  ASSERT_EQ(0x1cU, s.head());
  ASSERT_TRUE(s.isUTCDate());
  ASSERT_EQ(9UL, s.byteSize());
  ASSERT_EQ(value, s.getUTCDate());
}

TEST(BuilderTest, UTCDateMin) {
  int64_t const value = INT64_MIN;
  Builder b;
  b.add(Value(value, ValueType::UTCDate));

  Slice s(b.start());
  ASSERT_EQ(0x1cU, s.head());
  ASSERT_TRUE(s.isUTCDate());
  ASSERT_EQ(9UL, s.byteSize());
  ASSERT_EQ(value, s.getUTCDate());
}

TEST(BuilderTest, UTCDateMax) {
  int64_t const value = INT64_MAX;
  Builder b;
  b.add(Value(value, ValueType::UTCDate));

  Slice s(b.start());
  ASSERT_EQ(0x1cU, s.head());
  ASSERT_TRUE(s.isUTCDate());
  ASSERT_EQ(9UL, s.byteSize());
  ASSERT_EQ(value, s.getUTCDate());
}

TEST(BuilderTest, CustomTypeID) {
  // This is somewhat tautological, nevertheless...
  static uint8_t const correctResult[] = {0xf1, 0x2b, 0x78, 0x56, 0x34, 0x12,
                                          0x45, 0x02, 0x03, 0x05, 0x08, 0x0d};

  Builder b;
  uint8_t* p = b.add(ValuePair(sizeof(correctResult), ValueType::Custom));
  memcpy(p, correctResult, sizeof(correctResult));
  uint8_t* result = b.start();
  ValueLength len = b.size();

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));
}

TEST(BuilderTest, AddBCD) {
  Builder b;
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::BCD)),
                              Exception::NotImplemented);
}

TEST(BuilderTest, AddOnNonArray) {
  Builder b;
  b.add(Value(ValueType::Object));
  ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(true)),
                              Exception::BuilderKeyMustBeString);
}

TEST(BuilderTest, AddOnNonObject) {
  Builder b;
  b.add(Value(ValueType::Array));
  ASSERT_VELOCYPACK_EXCEPTION(b.add("foo", Value(true)),
                              Exception::BuilderNeedOpenObject);
}

TEST(BuilderTest, StartCalledOnOpenObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  ASSERT_VELOCYPACK_EXCEPTION(b.start(), Exception::BuilderNotSealed);
}

TEST(BuilderTest, StartCalledOnOpenObjectWithSubs) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(ValueType::Array));
  b.add(Value(1));
  b.add(Value(2));
  b.close();
  ASSERT_VELOCYPACK_EXCEPTION(b.start(), Exception::BuilderNotSealed);
}

TEST(BuilderTest, HasKeyNonObject) {
  Builder b;
  b.add(Value(1));
  ASSERT_VELOCYPACK_EXCEPTION(b.hasKey("foo"),
                              Exception::BuilderNeedOpenObject);
}

TEST(BuilderTest, HasKeyArray) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1));
  ASSERT_VELOCYPACK_EXCEPTION(b.hasKey("foo"),
                              Exception::BuilderNeedOpenObject);
}

TEST(BuilderTest, HasKeyEmptyObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  ASSERT_FALSE(b.hasKey("foo"));
  ASSERT_FALSE(b.hasKey("bar"));
  ASSERT_FALSE(b.hasKey("baz"));
  ASSERT_FALSE(b.hasKey("quetzalcoatl"));
  b.close();
}

TEST(BuilderTest, HasKeySubObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("foo", Value(1));
  b.add("bar", Value(true));
  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_FALSE(b.hasKey("baz"));

  b.add("bark", Value(ValueType::Object));
  ASSERT_FALSE(b.hasKey("bark"));
  ASSERT_FALSE(b.hasKey("foo"));
  ASSERT_FALSE(b.hasKey("bar"));
  ASSERT_FALSE(b.hasKey("baz"));
  b.close();

  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_TRUE(b.hasKey("bark"));
  ASSERT_FALSE(b.hasKey("baz"));

  b.add("baz", Value(42));
  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_TRUE(b.hasKey("bark"));
  ASSERT_TRUE(b.hasKey("baz"));
  b.close();
}

TEST(BuilderTest, HasKeyCompact) {
  Builder b;
  b.add(Value(ValueType::Object, true));
  b.add("foo", Value(1));
  b.add("bar", Value(true));
  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_FALSE(b.hasKey("baz"));

  b.add("bark", Value(ValueType::Object, true));
  ASSERT_FALSE(b.hasKey("bark"));
  ASSERT_FALSE(b.hasKey("foo"));
  ASSERT_FALSE(b.hasKey("bar"));
  ASSERT_FALSE(b.hasKey("baz"));
  b.close();

  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_TRUE(b.hasKey("bark"));
  ASSERT_FALSE(b.hasKey("baz"));

  b.add("baz", Value(42));
  ASSERT_TRUE(b.hasKey("foo"));
  ASSERT_TRUE(b.hasKey("bar"));
  ASSERT_TRUE(b.hasKey("bark"));
  ASSERT_TRUE(b.hasKey("baz"));
  b.close();
}

TEST(BuilderTest, GetKeyNonObject) {
  Builder b;
  b.add(Value(1));
  ASSERT_VELOCYPACK_EXCEPTION(b.getKey("foo"),
                              Exception::BuilderNeedOpenObject);
}

TEST(BuilderTest, GetKeyArray) {
  Builder b;
  b.add(Value(ValueType::Array));
  b.add(Value(1));
  ASSERT_VELOCYPACK_EXCEPTION(b.getKey("foo"),
                              Exception::BuilderNeedOpenObject);
}

TEST(BuilderTest, GetKeyEmptyObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  ASSERT_TRUE(b.getKey("foo").isNone());
  ASSERT_TRUE(b.getKey("bar").isNone());
  ASSERT_TRUE(b.getKey("baz").isNone());
  ASSERT_TRUE(b.getKey("quetzalcoatl").isNone());
  b.close();
}

TEST(BuilderTest, GetKeySubObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("foo", Value(1));
  b.add("bar", Value(true));
  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_TRUE(b.getKey("baz").isNone());

  b.add("bark", Value(ValueType::Object));
  ASSERT_TRUE(b.getKey("bark").isNone());
  ASSERT_TRUE(b.getKey("foo").isNone());
  ASSERT_TRUE(b.getKey("bar").isNone());
  ASSERT_TRUE(b.getKey("baz").isNone());
  b.close();

  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_TRUE(b.getKey("baz").isNone());
  ASSERT_TRUE(b.getKey("bark").isObject());

  b.add("baz", Value(42));
  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_EQ(42UL, b.getKey("baz").getUInt());
  ASSERT_TRUE(b.getKey("bark").isObject());
  b.close();
}

TEST(BuilderTest, GetKeyCompact) {
  Builder b;
  b.add(Value(ValueType::Object, true));
  b.add("foo", Value(1));
  b.add("bar", Value(true));
  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_TRUE(b.getKey("baz").isNone());

  b.add("bark", Value(ValueType::Object, true));
  ASSERT_FALSE(b.getKey("bark").isObject());
  ASSERT_TRUE(b.getKey("foo").isNone());
  ASSERT_TRUE(b.getKey("bar").isNone());
  ASSERT_TRUE(b.getKey("baz").isNone());
  ASSERT_TRUE(b.getKey("bark").isNone());
  b.close();

  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_TRUE(b.getKey("bark").isObject());
  ASSERT_TRUE(b.getKey("baz").isNone());

  b.add("baz", Value(42));
  ASSERT_EQ(1UL, b.getKey("foo").getUInt());
  ASSERT_TRUE(b.getKey("bar").getBool());
  ASSERT_TRUE(b.getKey("bark").isObject());
  ASSERT_EQ(42UL, b.getKey("baz").getUInt());
  b.close();
}

TEST(BuilderTest, IsClosedMixed) {
  Builder b;
  ASSERT_TRUE(b.isClosed());
  b.add(Value(ValueType::Null));
  ASSERT_TRUE(b.isClosed());
  b.add(Value(true));
  ASSERT_TRUE(b.isClosed());

  b.add(Value(ValueType::Array));
  ASSERT_FALSE(b.isClosed());

  b.add(Value(true));
  ASSERT_FALSE(b.isClosed());
  b.add(Value(true));
  ASSERT_FALSE(b.isClosed());

  b.close();
  ASSERT_TRUE(b.isClosed());

  b.add(Value(ValueType::Object));
  ASSERT_FALSE(b.isClosed());

  b.add("foo", Value(true));
  ASSERT_FALSE(b.isClosed());

  b.add("bar", Value(true));
  ASSERT_FALSE(b.isClosed());

  b.add("baz", Value(ValueType::Array));
  ASSERT_FALSE(b.isClosed());

  b.close();
  ASSERT_FALSE(b.isClosed());

  b.close();
  ASSERT_TRUE(b.isClosed());
}

TEST(BuilderTest, IsClosedObject) {
  Builder b;
  ASSERT_TRUE(b.isClosed());
  b.add(Value(ValueType::Object));
  ASSERT_FALSE(b.isClosed());

  b.add("foo", Value(true));
  ASSERT_FALSE(b.isClosed());

  b.add("bar", Value(true));
  ASSERT_FALSE(b.isClosed());

  b.add("baz", Value(ValueType::Object));
  ASSERT_FALSE(b.isClosed());

  b.close();
  ASSERT_FALSE(b.isClosed());

  b.close();
  ASSERT_TRUE(b.isClosed());
}

TEST(BuilderTest, CloseClosed) {
  Builder b;
  ASSERT_TRUE(b.isClosed());
  b.add(Value(ValueType::Object));
  ASSERT_FALSE(b.isClosed());
  b.close();

  ASSERT_VELOCYPACK_EXCEPTION(b.close(), Exception::BuilderNeedOpenCompound);
}

TEST(BuilderTest, Clone) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("foo", Value(true));
  b.add("bar", Value(false));
  b.add("baz", Value("foobarbaz"));
  b.close();

  Slice s1(b.start());
  Builder clone = b.clone(s1);
  ASSERT_NE(s1.start(), clone.start());

  Slice s2(clone.start());

  ASSERT_TRUE(s1.isObject());
  ASSERT_TRUE(s2.isObject());
  ASSERT_EQ(3UL, s1.length());
  ASSERT_EQ(3UL, s2.length());

  ASSERT_TRUE(s1.hasKey("foo"));
  ASSERT_TRUE(s2.hasKey("foo"));
  ASSERT_NE(s1.get("foo").start(), s2.get("foo").start());
  ASSERT_TRUE(s1.hasKey("bar"));
  ASSERT_TRUE(s2.hasKey("bar"));
  ASSERT_NE(s1.get("bar").start(), s2.get("bar").start());
  ASSERT_TRUE(s1.hasKey("baz"));
  ASSERT_TRUE(s2.hasKey("baz"));
  ASSERT_NE(s1.get("baz").start(), s2.get("baz").start());
}

TEST(BuilderTest, CloneDestroyOriginal) {
  Builder clone;  // empty
  {
    Builder b;
    b.add(Value(ValueType::Object));
    b.add("foo", Value(true));
    b.add("bar", Value(false));
    b.add("baz", Value("foobarbaz"));
    b.close();

    Slice s(b.start());
    clone = b.clone(s);
    ASSERT_NE(b.start(), clone.start());
    // now b goes out of scope. clone should survive!
  }

  Slice s(clone.start());
  ASSERT_TRUE(s.isObject());
  ASSERT_EQ(3UL, s.length());

  ASSERT_TRUE(s.hasKey("foo"));
  ASSERT_TRUE(s.get("foo").getBoolean());
  ASSERT_TRUE(s.hasKey("bar"));
  ASSERT_FALSE(s.get("bar").getBoolean());
  ASSERT_TRUE(s.hasKey("baz"));
  ASSERT_EQ("foobarbaz", s.get("baz").copyString());
}

TEST(BuilderTest, RemoveLastNonObject) {
  Builder b;
  b.add(Value(true));
  b.add(Value(false));
  ASSERT_VELOCYPACK_EXCEPTION(b.removeLast(),
                              Exception::BuilderNeedOpenCompound);
}

TEST(BuilderTest, RemoveLastSealed) {
  Builder b;
  ASSERT_VELOCYPACK_EXCEPTION(b.removeLast(),
                              Exception::BuilderNeedOpenCompound);
}

TEST(BuilderTest, RemoveLastEmptyObject) {
  Builder b;
  b.add(Value(ValueType::Object));

  ASSERT_VELOCYPACK_EXCEPTION(b.removeLast(), Exception::BuilderNeedSubvalue);
}

TEST(BuilderTest, RemoveLastObjectInvalid) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("foo", Value(true));
  b.removeLast();
  ASSERT_VELOCYPACK_EXCEPTION(b.removeLast(), Exception::BuilderNeedSubvalue);
}

TEST(BuilderTest, RemoveLastObject) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("foo", Value(true));
  b.add("bar", Value(false));

  b.removeLast();
  b.close();

  Slice s(b.start());
  ASSERT_TRUE(s.isObject());
  ASSERT_EQ(1UL, s.length());
  ASSERT_TRUE(s.hasKey("foo"));
  ASSERT_TRUE(s.get("foo").getBoolean());
  ASSERT_FALSE(s.hasKey("bar"));
}

TEST(BuilderTest, AttributeTranslations) {
  std::unique_ptr<AttributeTranslator> translator(new AttributeTranslator);

  translator->add("foo", 1);
  translator->add("bar", 2);
  translator->add("baz", 3);
  translator->add("bark", 4);
  translator->add("mötör", 5);
  translator->add("quetzalcoatl", 6);
  translator->seal();

  Options options;
  options.sortAttributeNames = false;
  options.attributeTranslator = translator.get();

  Builder b(&options);
  b.add(Value(ValueType::Object));
  b.add("foo", Value(true));
  b.add("bar", Value(false));
  b.add("baz", Value(1));
  b.add("bart", Value(2));
  b.add("bark", Value(42));
  b.add("mötör", Value(19));
  b.add("mötörhead", Value(20));
  b.add("quetzal", Value(21));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x0f, 0x35, 0x08, 0x31, 0x1a, 0x32, 0x19, 0x33, 0x31, 0x44, 0x62,
      0x61, 0x72, 0x74, 0x32, 0x34, 0x20, 0x2a, 0x35, 0x20, 0x13, 0x4b,
      0x6d, 0xc3, 0xb6, 0x74, 0xc3, 0xb6, 0x72, 0x68, 0x65, 0x61, 0x64,
      0x20, 0x14, 0x47, 0x71, 0x75, 0x65, 0x74, 0x7a, 0x61, 0x6c, 0x20,
      0x15, 0x03, 0x05, 0x07, 0x09, 0x0f, 0x12, 0x15, 0x23};

  ASSERT_EQ(sizeof(correctResult), len);
  ASSERT_EQ(0, memcmp(result, correctResult, len));

  Slice s = b.slice();

  ASSERT_TRUE(s.hasKey("foo"));
  ASSERT_TRUE(s.hasKey("bar"));
  ASSERT_TRUE(s.hasKey("baz"));
  ASSERT_TRUE(s.hasKey("bart"));
  ASSERT_TRUE(s.hasKey("bark"));
  ASSERT_TRUE(s.hasKey("mötör"));
  ASSERT_TRUE(s.hasKey("mötörhead"));
  ASSERT_TRUE(s.hasKey("quetzal"));
}

TEST(BuilderTest, AttributeTranslationsSorted) {
  std::unique_ptr<AttributeTranslator> translator(new AttributeTranslator);

  translator->add("foo", 1);
  translator->add("bar", 2);
  translator->add("baz", 3);
  translator->add("bark", 4);
  translator->add("mötör", 5);
  translator->add("quetzalcoatl", 6);
  translator->seal();

  Options options;
  options.sortAttributeNames = true;
  options.attributeTranslator = translator.get();

  Builder b(&options);
  b.add(Value(ValueType::Object));
  b.add("foo", Value(true));
  b.add("bar", Value(false));
  b.add("baz", Value(1));
  b.add("bart", Value(2));
  b.add("bark", Value(42));
  b.add("mötör", Value(19));
  b.add("mötörhead", Value(20));
  b.add("quetzal", Value(21));
  b.close();

  uint8_t* result = b.start();
  ValueLength len = b.size();

  static uint8_t correctResult[] = {
      0x0b, 0x35, 0x08, 0x31, 0x1a, 0x32, 0x19, 0x33, 0x31, 0x44, 0x62,
      0x61, 0x72, 0x74, 0x32, 0x34, 0x20, 0x2a, 0x35, 0x20, 0x13, 0x4b,
      0x6d, 0xc3, 0xb6, 0x74, 0xc3, 0xb6, 0x72, 0x68, 0x65, 0x61, 0x64,
      0x20, 0x14, 0x47, 0x71, 0x75, 0x65, 0x74, 0x7a, 0x61, 0x6c, 0x20,
      0x15, 0x05, 0x0f, 0x09, 0x07, 0x03, 0x12, 0x15, 0x23};

  ASSERT_EQ(sizeof(correctResult), len);

  ASSERT_EQ(0, memcmp(result, correctResult, len));
  Slice s = b.slice();

  ASSERT_TRUE(s.hasKey("foo"));
  ASSERT_TRUE(s.hasKey("bar"));
  ASSERT_TRUE(s.hasKey("baz"));
  ASSERT_TRUE(s.hasKey("bart"));
  ASSERT_TRUE(s.hasKey("bark"));
  ASSERT_TRUE(s.hasKey("mötör"));
  ASSERT_TRUE(s.hasKey("mötörhead"));
  ASSERT_TRUE(s.hasKey("quetzal"));
}

TEST(BuilderTest, AttributeTranslationsRollbackSet) {
  std::unique_ptr<AttributeTranslator> translator(new AttributeTranslator);

  translator->add("foo", 1);
  translator->seal();

  Options options;
  options.attributeTranslator = translator.get();

  Builder b(&options);
  b.add(Value(ValueType::Object));
  ASSERT_VELOCYPACK_EXCEPTION(b.add("foo", Value(ValueType::Custom)),
                              Exception::BuilderUnexpectedType);
  ASSERT_VELOCYPACK_EXCEPTION(b.add("FOOBAR", Value(ValueType::Custom)),
                              Exception::BuilderUnexpectedType);
}

TEST(BuilderTest, ToString) {
  Builder b;
  b.add(Value(ValueType::Object));
  b.add("test1", Value(123));
  b.add("test2", Value("foobar"));
  b.add("test3", Value(true));
  b.close();

  ASSERT_EQ("{\n  \"test1\" : 123,\n  \"test2\" : \"foobar\",\n  \"test3\" : true\n}",
            b.toString());
}

TEST(BuilderTest, ObjectBuilder) {
  Options options;
  options.sortAttributeNames = false;
  Builder b(&options);
  {
    ASSERT_TRUE(b.isClosed());

    ObjectBuilder ob(&b);
    ASSERT_EQ(&*ob, &b);
    ASSERT_FALSE(b.isClosed());
    ASSERT_FALSE(ob->isClosed());
    ob->add("foo", Value("aha"));
    ob->add("bar", Value("qux"));
    ASSERT_FALSE(ob->isClosed());
    ASSERT_FALSE(b.isClosed());
  }
  ASSERT_TRUE(b.isClosed());

  ASSERT_EQ("{\n  \"foo\" : \"aha\",\n  \"bar\" : \"qux\"\n}", b.toString());
}

TEST(BuilderTest, ObjectBuilderNested) {
  Options options;
  options.sortAttributeNames = false;
  Builder b(&options);
  {
    ASSERT_TRUE(b.isClosed());

    ObjectBuilder ob(&b);
    ASSERT_EQ(&*ob, &b);
    ASSERT_FALSE(b.isClosed());
    ASSERT_FALSE(ob->isClosed());
    ob->add("foo", Value("aha"));
    ob->add("bar", Value("qux"));
    {
      ObjectBuilder ob2(&b, "hans");
      ASSERT_EQ(&*ob2, &b);
      ASSERT_FALSE(ob2->isClosed());
      ASSERT_FALSE(ob->isClosed());
      ASSERT_FALSE(b.isClosed());
    
      ob2->add("bart", Value("a"));
      ob2->add("zoo", Value("b"));
    }
    {
      ObjectBuilder ob2(&b, std::string("foobar"));
      ASSERT_EQ(&*ob2, &b);
      ASSERT_FALSE(ob2->isClosed());
      ASSERT_FALSE(ob->isClosed());
      ASSERT_FALSE(b.isClosed());
    
      ob2->add("bark", Value(1));
      ob2->add("bonk", Value(2));
    }

    ASSERT_FALSE(ob->isClosed());
    ASSERT_FALSE(b.isClosed());
  }
  ASSERT_TRUE(b.isClosed());

  ASSERT_EQ("{\n  \"foo\" : \"aha\",\n  \"bar\" : \"qux\",\n  \"hans\" : {\n    \"bart\" : \"a\",\n    \"zoo\" : \"b\"\n  },\n  \"foobar\" : {\n    \"bark\" : 1,\n    \"bonk\" : 2\n  }\n}", b.toString());
}

TEST(BuilderTest, ArrayBuilder) {
  Options options;
  Builder b(&options);
  {
    ASSERT_TRUE(b.isClosed());

    ArrayBuilder ob(&b);
    ASSERT_EQ(&*ob, &b);
    ASSERT_FALSE(b.isClosed());
    ASSERT_FALSE(ob->isClosed());
    ob->add(Value("foo"));
    ob->add(Value("bar"));
    ASSERT_FALSE(ob->isClosed());
    ASSERT_FALSE(b.isClosed());
  }
  ASSERT_TRUE(b.isClosed());
  
  ASSERT_EQ("[\n  \"foo\",\n  \"bar\"\n]", b.toString());
}

TEST(BuilderTest, ArrayBuilderNested) {
  Options options;
  Builder b(&options);
  {
    ASSERT_TRUE(b.isClosed());

    ArrayBuilder ob(&b);
    ASSERT_EQ(&*ob, &b);
    ASSERT_FALSE(b.isClosed());
    ASSERT_FALSE(ob->isClosed());
    ob->add(Value("foo"));
    ob->add(Value("bar"));
    {
      ArrayBuilder ob2(&b);
      ASSERT_EQ(&*ob2, &b);
      ASSERT_FALSE(ob2->isClosed());
      ASSERT_FALSE(ob->isClosed());
      ASSERT_FALSE(b.isClosed());

      ob2->add(Value("bart"));
      ob2->add(Value("qux"));
    }
    {
      ArrayBuilder ob2(&b);
      ASSERT_EQ(&*ob2, &b);
      ASSERT_FALSE(ob2->isClosed());
      ASSERT_FALSE(ob->isClosed());
      ASSERT_FALSE(b.isClosed());

      ob2->add(Value(1));
      ob2->add(Value(2));
    }
    ASSERT_FALSE(ob->isClosed());
    ASSERT_FALSE(b.isClosed());
  }
  ASSERT_TRUE(b.isClosed());
  
  ASSERT_EQ("[\n  \"foo\",\n  \"bar\",\n  [\n    \"bart\",\n    \"qux\"\n  ],\n  [\n    1,\n    2\n  ]\n]", b.toString());
}

TEST(BuilderTest, AddKeysSeparately1) {
  Builder b;
  b.openObject();
  b.add(Value("name"));
  b.add(Value("Neunhoeffer"));
  b.add(Value("firstName"));
  b.add(Value("Max"));
  b.close();
  ASSERT_EQ(R"({"firstName":"Max","name":"Neunhoeffer"})", b.toJson());
}

TEST(BuilderTest, AddKeysSeparately2) {
  Builder b;

  b.openObject();
  b.add(Value("foo"));
  b.openArray();
  b.close();

  b.add(Value("bar"));
  b.openObject();
  b.close();

  b.add(Value("baz"));
  uint8_t buf[] = { 0x31 };
  Slice s(buf);
  b.add(s);

  b.add(Value("bumm"));
  Options options;
  options.clearBuilderBeforeParse = false;
  Parser p(b, &options);
  p.parse("[13]");
  b.close();
  ASSERT_EQ(R"({"bar":{},"baz":1,"bumm":[13],"foo":[]})", b.toJson());
}

TEST(BuilderTest, AddKeysSeparatelyFail) {
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(false)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Null)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Array)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::Object)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1.0)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(12, ValueType::UTCDate)),
                                Exception::BuilderKeyMustBeString);
  }
  uint8_t buf[] = { 0x31 };
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(&buf, ValueType::External)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::MinKey)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(ValueType::MaxKey)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(1)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(-112)),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(Value(113)),
                                Exception::BuilderKeyMustBeString);
  }
  Slice s(buf);
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.add(s),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.openObject(),
                                Exception::BuilderNeedOpenArray);
  }
  {
    Builder b;
    b.openObject();
    ASSERT_VELOCYPACK_EXCEPTION(b.openArray(),
                                Exception::BuilderNeedOpenArray);
  }
  {
    Builder b;
    b.openObject();
    Options opt;
    opt.clearBuilderBeforeParse = false;
    Parser p(b, &opt);
    ASSERT_VELOCYPACK_EXCEPTION(p.parse("[13]"),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    Options opt;
    opt.clearBuilderBeforeParse = false;
    Parser p(b, &opt);
    ASSERT_VELOCYPACK_EXCEPTION(p.parse("\"max\""),
                                Exception::BuilderKeyMustBeString);
  }
  {
    Builder b;
    b.openObject();
    b.add(Value("abc"));
    ASSERT_VELOCYPACK_EXCEPTION(b.add("abc", Value(1)),
                                Exception::BuilderKeyAlreadyWritten);
  }
}

int main(int argc, char* argv[]) {
  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
