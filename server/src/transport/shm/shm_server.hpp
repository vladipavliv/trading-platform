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
#include "transport/shm/shm_layout.hpp"
#include "transport/shm/shm_manager.hpp"
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

  explicit ShmServer(ServerBus &bus) : bus_{bus}, layout_{ShmManager::layout()} {}

  ~ShmServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    running_.store(true, std::memory_order_release);
    layout_.downstream.futex.store(1, std::memory_order_release);
    notifyClientConnected();
  }

  void stop() {
    LOG_DEBUG_SYSTEM("stop");
    running_.store(false, std::memory_order_release);
    layout_.upstream.futex.fetch_add(1, std::memory_order_release);
    utils::futexWake(layout_.upstream.futex);
  }

private:
  void waitForConnection() {
    LOG_INFO_SYSTEM("Waiting for client connection");
    AtomicBool flag;
    const auto futexVal = layout_.upstream.futex.load(std::memory_order_acquire);
    if (futexVal == 0) {
      utils::futexWait(layout_.upstream.futex, futexVal);
    }
  }

  void notifyClientConnected() {
    if (upstreamClb_) {
      upstreamClb_(ShmTransport::makeReader(layout_.upstream, ErrorBus{bus_.systemBus}));
    }
    if (downstreamClb_) {
      downstreamClb_(ShmTransport::makeWriter(layout_.downstream));
    }
  }

private:
  ServerBus &bus_;
  ShmLayout &layout_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  AtomicBool running_{false};
};
} // namespace hft::server

#endif // HFT_SERVER_SHMSERVER_HPP