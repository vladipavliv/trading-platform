/**
 * @author Vladimir Pavliv
 * @date 2025-04-24
 */

#ifndef HFT_TEST_ICLIENTDOMAINCONSUMER_HPP
#define HFT_TEST_ICLIENTDOMAINCONSUMER_HPP

#include "domain_types.hpp"
#include "types.hpp"

namespace hft::test {

/**
 * @brief Consumer for server domain types
 */
class IClientDomainConsumer {
public:
  virtual void post(CRef<Order> msg) = 0;
  virtual void post(CRef<OrderStatus> msg) = 0;
  virtual void post(CRef<LoginRequest> msg) = 0;
  virtual void post(CRef<TokenBindRequest> msg) = 0;
  virtual void post(CRef<LoginResponse> msg) = 0;
  virtual void post(CRef<TickerPrice> msg) = 0;
};
} // namespace hft::test

#endif // HFT_TEST_ICLIENTDOMAINCONSUMER_HPP
