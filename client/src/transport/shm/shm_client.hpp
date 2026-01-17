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
  using StreamClb = std::function<void(ShmTransport &&transport)>;
  using DatagramClb = std::function<void(ShmTransport &&transport)>;

  explicit ShmClient(ClientBus &bus) : bus_{bus} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) {}

  void start() { initialize(); }

  void stop() {
    LOG_INFO("ShmClient stop");
    ShmReactor::instance().stop();
    // TODO(self): Notify server we are disconnecting
  }

private:
  void initialize() {
    LOG_DEBUG("ShmClient::initialize");
    if (upstreamClb_) {
      const auto name = Config::get<String>("shm.shm_upstream");
      upstreamClb_(ShmTransport::makeWriter(name, false));
    }
    if (downstreamClb_) {
      const auto name = Config::get<String>("shm.shm_downstream");
      downstreamClb_(ShmTransport::makeReader(name, false));
    }
    ShmReactor::instance().run();
  }

private:
  ClientBus &bus_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP