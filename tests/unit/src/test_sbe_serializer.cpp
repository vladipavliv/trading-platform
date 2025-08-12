/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#include <gtest/gtest.h>

#include "domain_types.hpp"
#include "serialization/sbe/sbe_domain_serializer.hpp"
#include "spies/consumer_spy.hpp"

namespace hft::tests {
using namespace serialization::sbe;
using namespace hft::serialization::gen::sbe;

TEST(SbeSerializerTest, serializeDeserialize) {
  ConsumerSpy spy;

  LoginRequest request{"name", "password"};
  ByteBuffer buffer;

  SbeDomainSerializer::serialize(request, buffer);
  size_t size = SbeDomainSerializer::deserialize<ConsumerSpy>(buffer.data(), buffer.size(), spy);

  ASSERT_TRUE(size != 0);
  ASSERT_TRUE(spy.checkValue(0, request));
  spy.printAll();
}

TEST(SbeSerializerTest, SeveralMessagesInOneBuffer) {
  ConsumerSpy spy;

  LoginRequest request0{"name0", "password0"};
  LoginRequest request1{"name1", "password1"};
  ByteBuffer buffer0;
  ByteBuffer buffer1;

  SbeDomainSerializer::serialize(request0, buffer0);
  SbeDomainSerializer::serialize(request1, buffer1);

  ByteBuffer buffer;
  buffer.reserve(buffer0.size() + buffer1.size());
  buffer.insert(buffer.end(), buffer0.begin(), buffer0.end());
  buffer.insert(buffer.end(), buffer1.begin(), buffer1.end());

  const auto ptr0 = buffer.data();
  const auto size0 = buffer.size();

  const auto ptr1 = buffer.data() + domain::LoginRequest::sbeBlockAndHeaderLength();
  const auto size1 = buffer.size() - domain::LoginRequest::sbeBlockAndHeaderLength();

  const size_t serSize0 = SbeDomainSerializer::deserialize<ConsumerSpy>(ptr0, size0, spy);
  const size_t serSize1 = SbeDomainSerializer::deserialize<ConsumerSpy>(ptr1, size1, spy);

  ASSERT_TRUE(serSize0 == domain::LoginRequest::sbeBlockAndHeaderLength());
  ASSERT_TRUE(serSize1 == domain::LoginRequest::sbeBlockAndHeaderLength());

  ASSERT_TRUE(spy.checkValue(0, request0));
  ASSERT_TRUE(spy.checkValue(1, request1));

  spy.printAll();
}

} // namespace hft::tests