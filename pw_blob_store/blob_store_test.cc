// Copyright 2020 The Pigweed Authors
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

#include "pw_blob_store/blob_store.h"

#include <array>
#include <cstddef>
#include <cstring>
#include <span>

#include "gtest/gtest.h"
#include "pw_kvs/crc16_checksum.h"
#include "pw_kvs/fake_flash_memory.h"
#include "pw_kvs/flash_memory.h"
#include "pw_kvs/test_key_value_store.h"
#include "pw_log/log.h"
#include "pw_random/xor_shift.h"

namespace pw::blob_store {
namespace {

class BlobStoreTest : public ::testing::Test {
 protected:
  BlobStoreTest() : flash_(kFlashAlignment), partition_(&flash_) {}

  void InitFlashTo(std::span<const std::byte> contents) {
    partition_.Erase();
    std::memcpy(flash_.buffer().data(), contents.data(), contents.size());
  }

  void InitSourceBufferToRandom(uint64_t seed,
                                size_t init_size_bytes = kBlobDataSize) {
    ASSERT_LE(init_size_bytes, source_buffer_.size());
    random::XorShiftStarRng64 rng(seed);

    std::memset(source_buffer_.data(),
                static_cast<int>(flash_.erased_memory_content()),
                source_buffer_.size());
    rng.Get(std::span(source_buffer_).first(init_size_bytes));
  }

  void InitSourceBufferToFill(char fill,
                              size_t fill_size_bytes = kBlobDataSize) {
    ASSERT_LE(fill_size_bytes, source_buffer_.size());
    std::memset(source_buffer_.data(),
                static_cast<int>(flash_.erased_memory_content()),
                source_buffer_.size());
    std::memset(source_buffer_.data(), fill, fill_size_bytes);
  }

  // Fill the source buffer with random pattern based on given seed, written to
  // BlobStore in specified chunk size.
  void WriteTestBlock(size_t write_size_bytes = kBlobDataSize) {
    ASSERT_LE(write_size_bytes, source_buffer_.size());
    constexpr size_t kBufferSize = 256;
    kvs::ChecksumCrc16 checksum;

    ConstByteSpan write_data =
        std::span(source_buffer_).first(write_size_bytes);

    char name[16] = {};
    snprintf(name, sizeof(name), "TestBlobBlock");

    BlobStoreBuffer<kBufferSize> blob(
        name, partition_, &checksum, kvs::TestKvs());
    EXPECT_EQ(Status::Ok(), blob.Init());

    BlobStore::BlobWriter writer(blob);
    EXPECT_EQ(Status::OK, writer.Open());
    ASSERT_EQ(Status::OK, writer.Write(write_data));
    EXPECT_EQ(Status::OK, writer.Close());

    // Use reader to check for valid data.
    BlobStore::BlobReader reader(blob);
    ASSERT_EQ(Status::Ok(), reader.Open());
    Result<ConstByteSpan> result = reader.GetMemoryMappedBlob();
    ASSERT_TRUE(result.ok());
    EXPECT_EQ(write_size_bytes, result.value().size_bytes());
    VerifyFlash(result.value());
    VerifyFlash(flash_.buffer());
    EXPECT_EQ(Status::OK, reader.Close());
  }

  // Open a new blob instance and read the blob using the given read chunk size.
  void ChunkReadTest(size_t read_chunk_size) {
    kvs::ChecksumCrc16 checksum;

    VerifyFlash(flash_.buffer());

    char name[16] = "TestBlobBlock";
    BlobStoreBuffer<16> blob(name, partition_, &checksum, kvs::TestKvs());
    EXPECT_EQ(Status::Ok(), blob.Init());

    // Use reader to check for valid data.
    BlobStore::BlobReader reader1(blob);
    ASSERT_EQ(Status::Ok(), reader1.Open());
    Result<ConstByteSpan> possible_blob = reader1.GetMemoryMappedBlob();
    ASSERT_TRUE(possible_blob.ok());
    VerifyFlash(possible_blob.value());
    EXPECT_EQ(Status::Ok(), reader1.Close());

    BlobStore::BlobReader reader(blob);
    ASSERT_EQ(Status::Ok(), reader.Open());

    std::array<std::byte, kBlobDataSize> read_buffer_;

    ByteSpan read_span = read_buffer_;
    while (read_span.size_bytes() > 0) {
      size_t read_size = std::min(read_span.size_bytes(), read_chunk_size);

      PW_LOG_DEBUG("Do write of %u bytes, %u bytes remain",
                   static_cast<unsigned>(read_size),
                   static_cast<unsigned>(read_span.size_bytes()));

      ASSERT_EQ(read_span.size_bytes(), reader.ConservativeReadLimit());
      auto result = reader.Read(read_span.first(read_size));
      ASSERT_EQ(result.status(), Status::Ok());
      read_span = read_span.subspan(read_size);
    }
    EXPECT_EQ(Status::Ok(), reader.Close());

    VerifyFlash(read_buffer_);
  }

  void VerifyFlash(ConstByteSpan verify_bytes, size_t offset = 0) {
    // Should be defined as same size.
    EXPECT_EQ(source_buffer_.size(), flash_.buffer().size_bytes());

    // Can't allow it to march off the end of source_buffer_.
    ASSERT_LE((verify_bytes.size_bytes() + offset), source_buffer_.size());

    for (size_t i = 0; i < verify_bytes.size_bytes(); i++) {
      ASSERT_EQ(source_buffer_[i + offset], verify_bytes[i]);
    }
  }

  static constexpr size_t kFlashAlignment = 16;
  static constexpr size_t kSectorSize = 2048;
  static constexpr size_t kSectorCount = 2;
  static constexpr size_t kBlobDataSize = (kSectorCount * kSectorSize);

  kvs::FakeFlashMemoryBuffer<kSectorSize, kSectorCount> flash_;
  kvs::FlashPartition partition_;
  std::array<std::byte, kBlobDataSize> source_buffer_;
};

TEST_F(BlobStoreTest, Init_Ok) {
  // TODO: Do init test with flash/kvs explicitly in the different possible
  // entry states.
  BlobStoreBuffer<256> blob("Blob_OK", partition_, nullptr, kvs::TestKvs());
  EXPECT_EQ(Status::Ok(), blob.Init());
}

TEST_F(BlobStoreTest, Discard) {
  InitSourceBufferToRandom(0x8675309);
  WriteTestBlock();
  constexpr char blob_title[] = "TestBlobBlock";
  std::array<std::byte, 64> tmp_buffer = {};

  kvs::ChecksumCrc16 checksum;

  // TODO: Do this test with flash/kvs in the different entry state
  // combinations.

  BlobStoreBuffer<256> blob(blob_title, partition_, &checksum, kvs::TestKvs());
  EXPECT_EQ(Status::OK, blob.Init());

  BlobStore::BlobWriter writer(blob);

  EXPECT_EQ(Status::OK, writer.Open());
  EXPECT_EQ(Status::OK, writer.Write(tmp_buffer));

  // The write does an implicit erase so there should be no key for this blob.
  EXPECT_EQ(Status::NOT_FOUND,
            kvs::TestKvs().Get(blob_title, tmp_buffer).status());
  EXPECT_EQ(Status::OK, writer.Close());

  EXPECT_EQ(Status::OK, kvs::TestKvs().Get(blob_title, tmp_buffer).status());

  EXPECT_EQ(Status::OK, writer.Open());
  EXPECT_EQ(Status::OK, writer.Discard());
  EXPECT_EQ(Status::OK, writer.Close());

  EXPECT_EQ(Status::NOT_FOUND,
            kvs::TestKvs().Get(blob_title, tmp_buffer).status());
}

TEST_F(BlobStoreTest, MultipleErase) {
  BlobStoreBuffer<256> blob("Blob_OK", partition_, nullptr, kvs::TestKvs());
  EXPECT_EQ(Status::Ok(), blob.Init());

  BlobStore::BlobWriter writer(blob);
  EXPECT_EQ(Status::Ok(), writer.Open());

  EXPECT_EQ(Status::Ok(), writer.Erase());
  EXPECT_EQ(Status::Ok(), writer.Erase());
  EXPECT_EQ(Status::Ok(), writer.Erase());
}

TEST_F(BlobStoreTest, OffsetRead) {
  InitSourceBufferToRandom(0x11309);
  WriteTestBlock();

  constexpr size_t kOffset = 10;
  ASSERT_LT(kOffset, kBlobDataSize);

  kvs::ChecksumCrc16 checksum;

  char name[16] = "TestBlobBlock";
  BlobStoreBuffer<16> blob(name, partition_, &checksum, kvs::TestKvs());
  EXPECT_EQ(Status::Ok(), blob.Init());
  BlobStore::BlobReader reader(blob);
  ASSERT_EQ(Status::Ok(), reader.Open(kOffset));

  std::array<std::byte, kBlobDataSize - kOffset> read_buffer_;
  ByteSpan read_span = read_buffer_;
  ASSERT_EQ(read_span.size_bytes(), reader.ConservativeReadLimit());

  auto result = reader.Read(read_span);
  ASSERT_EQ(result.status(), Status::Ok());
  EXPECT_EQ(Status::Ok(), reader.Close());
  VerifyFlash(read_buffer_, kOffset);
}

TEST_F(BlobStoreTest, InvalidReadOffset) {
  InitSourceBufferToRandom(0x11309);
  WriteTestBlock();

  constexpr size_t kOffset = kBlobDataSize;

  kvs::ChecksumCrc16 checksum;

  char name[16] = "TestBlobBlock";
  BlobStoreBuffer<16> blob(name, partition_, &checksum, kvs::TestKvs());
  EXPECT_EQ(Status::Ok(), blob.Init());
  BlobStore::BlobReader reader(blob);
  ASSERT_EQ(Status::InvalidArgument(), reader.Open(kOffset));
}

TEST_F(BlobStoreTest, ChunkRead1) {
  InitSourceBufferToRandom(0x8675309);
  WriteTestBlock();
  ChunkReadTest(1);
}

TEST_F(BlobStoreTest, ChunkRead3) {
  InitSourceBufferToFill(0);
  WriteTestBlock();
  ChunkReadTest(3);
}

TEST_F(BlobStoreTest, ChunkRead4) {
  InitSourceBufferToFill(1);
  WriteTestBlock();
  ChunkReadTest(4);
}

TEST_F(BlobStoreTest, ChunkRead5) {
  InitSourceBufferToFill(0xff);
  WriteTestBlock();
  ChunkReadTest(5);
}

TEST_F(BlobStoreTest, ChunkRead16) {
  InitSourceBufferToRandom(0x86);
  WriteTestBlock();
  ChunkReadTest(16);
}

TEST_F(BlobStoreTest, ChunkRead64) {
  InitSourceBufferToRandom(0x9);
  WriteTestBlock();
  ChunkReadTest(64);
}

TEST_F(BlobStoreTest, ChunkReadFull) {
  InitSourceBufferToRandom(0x9);
  WriteTestBlock();
  ChunkReadTest(kBlobDataSize);
}

TEST_F(BlobStoreTest, PartialBufferThenClose) {
  // Do write of only a partial chunk, which will only have bytes in buffer
  // (none written to flash) at close.
  size_t data_bytes = 12;
  InitSourceBufferToRandom(0x111, data_bytes);
  WriteTestBlock(data_bytes);

  // Do write with several full chunks and then some partial.
  data_bytes = 158;
  InitSourceBufferToRandom(0x3222, data_bytes);
}

// Test to do write/close, write/close multiple times.
TEST_F(BlobStoreTest, MultipleWrites) {
  InitSourceBufferToRandom(0x1121);
  WriteTestBlock();
  InitSourceBufferToRandom(0x515);
  WriteTestBlock();
  InitSourceBufferToRandom(0x4321);
  WriteTestBlock();
}

}  // namespace
}  // namespace pw::blob_store
