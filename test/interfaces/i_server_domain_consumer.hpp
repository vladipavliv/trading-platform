/**
 * @author Vladimir Pavliv
 * @date 2025-04-24
 */

#ifndef HFT_TEST_ISERVERDOMAINCONSUMER_HPP
#define HFT_TEST_ISERVERDOMAINCONSUMER_HPP

#include "domain_types.hpp"
#include "server_types.hpp"
#include "types.hpp"

namespace hft::test {

/**
 * @brief Consumer for server domain types
 */
class IServerDomainConsumer {
public:
  virtual void post(CRef<server::ServerOrder> msg) = 0;
  virtual void post(CRef<server::ServerOrderStatus> msg) = 0;
  virtual void post(CRef<server::ServerLoginRequest> msg) = 0;
  virtual void post(CRef<server::ServerTokenBindRequest> msg) = 0;
  virtual void post(CRef<server::ServerLoginResponse> msg) = 0;
  virtual void post(CRef<TickerPrice> msg) = 0;
};
} // namespace hft::test

#endif // HFT_TEST_ISERVERDOMAINCONSUMER_HPP
