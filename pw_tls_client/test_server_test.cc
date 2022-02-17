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

#include "pw_tls_client/test/test_server.h"

#include <span>
#include <string>

#include "gtest/gtest.h"

// The following header contains a set of test certificates and keys.
// It is generated by
// third_party/boringssl/py/boringssl/generate_test_data.py.
#include "test_certs_and_keys.h"

#define ASSERT_OK(expr) ASSERT_EQ(pw::OkStatus(), expr)

namespace pw::tls_client::test {
namespace {

int TestClientBioRead(BIO* bio, char* out, int outl) {
  auto read_writer = static_cast<stream::ReaderWriter*>(bio->ptr);
  auto res = read_writer->Read(out, outl);
  if (!res.ok()) {
    return -1;
  }
  if (res.value().empty()) {
    BIO_set_retry_read(bio);
    return -1;
  }
  return res.value().size();
}

int TestClientBioWrite(BIO* bio, const char* in, int inl) {
  auto read_writer = static_cast<stream::ReaderWriter*>(bio->ptr);
  auto res = read_writer->Write(in, inl);
  if (!res.ok()) {
    return -1;
  }
  return inl;
}

int TestClientBioNew(BIO* bio) {
  bio->init = 1;
  return 1;
}

long TestClientBioCtrl(BIO*, int, long, void*) { return 1; }

int TestClientBioFree(BIO*) { return 1; }

const BIO_METHOD bio_method = {
    BIO_TYPE_MEM,
    "bio test server test",
    TestClientBioWrite,
    TestClientBioRead,
    nullptr,
    nullptr,
    TestClientBioCtrl,
    TestClientBioNew,
    TestClientBioFree,
    nullptr,
};

// Server needs to send certificate. Thus the send buffer needs to be bigger.
std::array<std::byte, 4096> server_send_buffer;
std::array<std::byte, 512> server_receive_buffer;

// Create a raw BoringSSL client and load test trust anchors.
void CreateSSLClient(bssl::UniquePtr<SSL_CTX>* ctx,
                     bssl::UniquePtr<SSL>* client,
                     stream::ReaderWriter* read_writer) {
  *ctx = bssl::UniquePtr<SSL_CTX>(SSL_CTX_new(TLS_method()));
  ASSERT_NE(*ctx, nullptr);
  *client = bssl::UniquePtr<SSL>(SSL_new(ctx->get()));
  ASSERT_NE(*client, nullptr);
  BIO* bio = BIO_new(&bio_method);
  ASSERT_NE(bio, nullptr);

  // Load trust anchors to client
  auto store = SSL_CTX_get_cert_store(ctx->get());
  X509_VERIFY_PARAM_clear_flags(store->param, X509_V_FLAG_USE_CHECK_TIME);
  const pw::ConstByteSpan kTrustAnchors[] = {kRootACert, kRootBCert};
  for (auto cert : kTrustAnchors) {
    auto res = ParseDerCertificate(cert);
    ASSERT_OK(res.status());
    ASSERT_EQ(X509_STORE_add_cert(store, res.value()), 1);
    X509_free(res.value());
  }
  bio->ptr = read_writer;
  SSL_set_bio(client->get(), bio, bio);
}

}  // namespace

TEST(InMemoryTestServer, NormalConnectionSucceed) {
  InMemoryTestServer server(server_receive_buffer, server_send_buffer);
  const ConstByteSpan kIntermediates[] = {kSubCACert};
  ASSERT_OK(server.Initialize(kServerKey, kServerCert, kIntermediates));

  // Create a raw BoringSSL client
  bssl::UniquePtr<SSL_CTX> client_ctx;
  bssl::UniquePtr<SSL> ssl_client;
  CreateSSLClient(&client_ctx, &ssl_client, &server);

  // Handshake should be OK
  ASSERT_EQ(SSL_connect(ssl_client.get()), 1);
  ASSERT_TRUE(server.SessionEstablished());

  // Client should pass certificate verification.
  ASSERT_EQ(SSL_get_verify_result(ssl_client.get()), 0);

  // Send some data to server
  const char send_expected[] = "hello";
  int send_len =
      SSL_write(ssl_client.get(), send_expected, sizeof(send_expected));
  ASSERT_EQ(static_cast<size_t>(send_len), sizeof(send_expected));

  char receive_actual[sizeof(send_expected) + 1] = {0};
  int read_ret =
      SSL_read(ssl_client.get(), receive_actual, sizeof(receive_actual));
  ASSERT_EQ(static_cast<size_t>(read_ret), sizeof(send_expected));
  ASSERT_STREQ(send_expected, receive_actual);

  // Shutdown
  EXPECT_FALSE(server.ClientShutdownReceived());
  ASSERT_NE(SSL_shutdown(ssl_client.get()), -1);
  ASSERT_TRUE(server.ClientShutdownReceived());
}

TEST(InMemoryTestServer, BufferTooSmallErrorsOut) {
  std::array<std::byte, 1> insufficient_buffer;
  InMemoryTestServer server(server_receive_buffer, insufficient_buffer);
  const ConstByteSpan kIntermediates[] = {kSubCACert};
  ASSERT_OK(server.Initialize(kServerKey, kServerCert, kIntermediates));

  // Create a raw BoringSSL client
  bssl::UniquePtr<SSL_CTX> client_ctx;
  bssl::UniquePtr<SSL> ssl_client;
  CreateSSLClient(&client_ctx, &ssl_client, &server);

  // Handshake should fail as server shouldn't have enough buffer
  ASSERT_NE(SSL_connect(ssl_client.get()), 1);
  ASSERT_EQ(server.GetLastBioStatus(), Status::ResourceExhausted());
}

}  // namespace pw::tls_client::test
