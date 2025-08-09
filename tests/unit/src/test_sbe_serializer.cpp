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

TEST(SbeSerializerTest, serializeDeserialize) {
  LoginRequest request{"name", "password"};
  ByteBuffer buffer;
  ConsumerSpy spy;

  SbeDomainSerializer::serialize(request, buffer);
  auto ok = SbeDomainSerializer::deserialize<ConsumerSpy>(buffer.data(), buffer.size(), spy);

  ASSERT_TRUE(spy.checkValue(0, request));
  spy.printAll();
}

} // namespace hft::tests