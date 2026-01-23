/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_SERVER_SHMSERVER_HPP
#define HFT_SERVER_SHMSERVER_HPP

#include <functional>
#include <memory>
#include <string>

#include "bus/bus_hub.hpp"
#include "config/server_config.hpp"
#include "events.hpp"
#include "traits.hpp"
#include "transport/shm/shm_reactor.hpp"
#include "transport/shm/shm_transport.hpp"
#include "utils/memory_utils.hpp"
#include "utils/sync_utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class ShmServer {
public:
  using ShmHandler = MoveHandler<ShmTransport>;

  explicit ShmServer(Context &ctx)
      : ctx_{ctx}, reactor_{ctx_.config.data, ctx_.stopToken, ErrorBus{ctx_.bus.systemBus}} {}

  ~ShmServer() { LOG_DEBUG_SYSTEM("~ShmServer"); }

  void setUpstreamClb(ShmHandler &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(ShmHandler &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(ShmHandler &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    if (reactor_.running()) {
      LOG_ERROR_SYSTEM("ShmServer is already running");
      return;
    }
    initialize();
    reactor_.run([this]() { ctx_.bus.post(ComponentReady{Component::Ipc}); });
  }

  void stop() {
    LOG_DEBUG_SYSTEM("Stopping ShmServer");
    reactor_.stop();
  }

private:
  void initialize() {
    LOG_DEBUG("ShmServer::initialize");
    if (upstreamClb_) {
      const auto name = ctx_.config.data.get<String>("shm.shm_upstream");
      upstreamClb_(ShmTransport::makeReader(name));
    }
    if (downstreamClb_) {
      const auto name = ctx_.config.data.get<String>("shm.shm_downstream");
      downstreamClb_(ShmTransport::makeWriter(name));
    }
  }

private:
  Context &ctx_;

  ShmReactor reactor_;

  ShmHandler upstreamClb_;
  ShmHandler downstreamClb_;
  ShmHandler datagramClb_;
};
} // namespace hft::server

#endif // HFT_SERVER_SHMSERVER_HPP