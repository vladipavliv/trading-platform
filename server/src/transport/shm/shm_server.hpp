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
#include "traits.hpp"
#include "transport/async_transport.hpp"
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
  using StreamClb = std::function<void(ShmTransport &&transport)>;
  using DatagramClb = std::function<void(ShmTransport &&transport)>;

  explicit ShmServer(ServerBus &bus) : bus_{bus}, reactor_{ErrorBus{bus_.systemBus}} {}

  ~ShmServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() { initialize(); }

  void stop() {
    LOG_DEBUG_SYSTEM("stop");
    reactor_.stop();
    // TODO(self): properly signal client we are closing
  }

private:
  void initialize() {
    LOG_DEBUG("ShmServer::initialize");
    if (upstreamClb_) {
      const auto name = Config::get<String>("shm.shm_upstream");
      upstreamClb_(ShmTransport::makeReader(name));
    }
    if (downstreamClb_) {
      const auto name = Config::get<String>("shm.shm_downstream");
      downstreamClb_(ShmTransport::makeWriter(name));
    }
    reactor_.run();
  }

private:
  ServerBus &bus_;

  ShmReactor reactor_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;
};
} // namespace hft::server

#endif // HFT_SERVER_SHMSERVER_HPP