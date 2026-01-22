/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_CLIENT_SHMCLIENT_HPP
#define HFT_CLIENT_SHMCLIENT_HPP

#include "bus/bus_hub.hpp"
#include "config/client_config.hpp"
#include "primitive_types.hpp"
#include "traits.hpp"
#include "transport/shm/shm_reactor.hpp"
#include "transport/shm/shm_transport.hpp"
#include "utils/memory_utils.hpp"

namespace hft::client {

/**
 * @brief
 */
class ShmClient {
public:
  using ShmHandler = MoveHandler<ShmTransport>;

  explicit ShmClient(Context &ctx)
      : ctx_{ctx}, reactor_{ctx.config.data, ctx.stopToken, ErrorBus{ctx.bus.systemBus}} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(ShmHandler &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(ShmHandler &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(ShmHandler &&datagramClb) {}

  void start() { initialize(); }

  void stop() {
    LOG_INFO("ShmClient stop");
    reactor_.stop();
  }

private:
  void initialize() {
    LOG_DEBUG("ShmClient::initialize");
    if (upstreamClb_) {
      const auto name = ctx_.config.data.get<String>("shm.shm_upstream");
      upstreamClb_(ShmTransport::makeWriter(name));
    }
    if (downstreamClb_) {
      const auto name = ctx_.config.data.get<String>("shm.shm_downstream");
      downstreamClb_(ShmTransport::makeReader(name));
    }
    reactor_.run();
  }

private:
  Context &ctx_;

  ShmReactor reactor_;

  ShmHandler upstreamClb_;
  ShmHandler downstreamClb_;
  ShmHandler datagramClb_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP