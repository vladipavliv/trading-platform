/**
 * @author Vladimir Pavliv
 * @date 2025-08-09
 */

#include <gtest/gtest.h>

#include "domain_types.hpp"
#include "serialization/fbs/fbs_domain_serializer.hpp"
#include "spies/consumer_spy.hpp"

namespace hft::tests {
using namespace serialization::fbs;
using namespace hft::serialization::gen::fbs;

TEST(FbsSerializerTest, serializeDeserialize) {
  ConsumerSpy spy;

  LoginRequest request{"name", "password"};
  ByteBuffer buffer;

  const size_t serializedSIze = FbsDomainSerializer::serialize(request, buffer);
  ASSERT_TRUE(serializedSIze != 0);
  ASSERT_EQ(buffer.size(), serializedSIze);

  const size_t deserializedSize =
      FbsDomainSerializer::deserialize<ConsumerSpy>(buffer.data(), buffer.size(), spy);
  ASSERT_EQ(deserializedSize, serializedSIze);

  ASSERT_EQ(spy.size(), 1);
  ASSERT_TRUE(spy.checkValue(0, request));
  spy.printAll();
}

} // namespace hft::tests