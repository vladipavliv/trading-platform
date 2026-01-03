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
  ByteBuffer buffer(128);

  const size_t serSize = FbsDomainSerializer::serialize(request, buffer.data());
  ASSERT_TRUE(serSize != 0);

  const size_t deserSize =
      FbsDomainSerializer::deserialize<ConsumerSpy>(buffer.data(), serSize, spy);
  ASSERT_EQ(deserSize, serSize);

  ASSERT_EQ(spy.size(), 1);
  ASSERT_TRUE(spy.checkValue(0, request));
  spy.printAll();
}

} // namespace hft::tests