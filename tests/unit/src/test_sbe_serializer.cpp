/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#include <gtest/gtest.h>

#include "container_types.hpp"
#include "domain_types.hpp"
#include "serialization/sbe/sbe_domain_serializer.hpp"
#include "utils/post_spy.hpp"

namespace hft::tests {
using namespace serialization::sbe;
using namespace hft::serialization::gen::sbe;

TEST(SbeSerializerTest, serializeDeserialize) {
  PostSpy spy;

  LoginRequest request{"name", "password"};
  ByteBuffer buffer(128);

  const size_t serSize = SbeDomainSerializer::serialize(request, buffer.data());
  ASSERT_TRUE(serSize != 0);

  auto deserSize = SbeDomainSerializer::deserialize<PostSpy>(buffer.data(), serSize, spy);

  ASSERT_TRUE(deserSize);
  ASSERT_TRUE(*deserSize != 0);
  ASSERT_TRUE(spy.checkValue(0, request));
  spy.printAll();
}

TEST(SbeSerializerTest, SeveralMessagesInOneBuffer) {
  PostSpy spy;

  LoginRequest request0{"name0", "password0"};
  LoginRequest request1{"name1", "password1"};
  ByteBuffer buffer0(128);
  ByteBuffer buffer1(128);

  const size_t serSize0 = SbeDomainSerializer::serialize(request0, buffer0.data());
  const size_t serSize1 = SbeDomainSerializer::serialize(request1, buffer1.data());

  ByteBuffer buffer;
  buffer.reserve(serSize0 + serSize1);
  buffer.insert(buffer.end(), buffer0.data(), buffer0.data() + serSize0);
  buffer.insert(buffer.end(), buffer1.data(), buffer1.data() + serSize1);

  const auto ptr0 = buffer.data();
  const auto size0 = buffer.size();

  const auto ptr1 = buffer.data() + domain::LoginRequest::sbeBlockAndHeaderLength();
  const auto size1 = buffer.size() - domain::LoginRequest::sbeBlockAndHeaderLength();

  const auto deserSize0 = SbeDomainSerializer::deserialize<PostSpy>(ptr0, size0, spy);
  const auto deserSize1 = SbeDomainSerializer::deserialize<PostSpy>(ptr1, size1, spy);

  ASSERT_TRUE(deserSize0);
  ASSERT_TRUE(deserSize1);

  ASSERT_TRUE(serSize0 == domain::LoginRequest::sbeBlockAndHeaderLength());
  ASSERT_TRUE(serSize1 == domain::LoginRequest::sbeBlockAndHeaderLength());

  ASSERT_TRUE(*deserSize0 == domain::LoginRequest::sbeBlockAndHeaderLength());
  ASSERT_TRUE(*deserSize1 == domain::LoginRequest::sbeBlockAndHeaderLength());

  ASSERT_TRUE(spy.checkValue(0, request0));
  ASSERT_TRUE(spy.checkValue(1, request1));

  spy.printAll();
}

} // namespace hft::tests