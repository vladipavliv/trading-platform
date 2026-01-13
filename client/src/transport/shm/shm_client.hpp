/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_CLIENT_SHMCLIENT_HPP
#define HFT_CLIENT_SHMCLIENT_HPP

#include <functional>
#include <memory>
#include <string>

#include "bus/bus_hub.hpp"
#include "config/client_config.hpp"
#include "internal_error.hpp"
#include "traits.hpp"
#include "transport/shm/shm_layout.hpp"
#include "transport/shm/shm_manager.hpp"
#include "transport/shm/shm_transport.hpp"
#include "utils/memory_utils.hpp"
#include "utils/thread_utils.hpp"

namespace hft::client {

/**
 * @brief
 */
class ShmClient {
public:
  using StreamClb = std::function<void(ShmTransport &&transport)>;
  using DatagramClb = std::function<void(ShmTransport &&transport)>;

  explicit ShmClient(ClientBus &bus) : bus_{bus}, layout_{ShmManager::layout()} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) {}

  void start() {
    if (running_) {
      LOG_ERROR_SYSTEM("ShmClient is already running");
      return;
    }
    if (layout_.downstream.futex.load(std::memory_order_acquire) == 0) {
      throw std::runtime_error("Server is offline");
    }
    layout_.upstream.futex.store(1, std::memory_order_release);
    utils::futexWake(layout_.upstream.futex);
    notifyConnected();
  }

  void stop() {
    LOG_INFO("ShmClient stop");
    running_.store(false, std::memory_order_release);
  }

private:
  void notifyConnected() {
    if (upstreamClb_) {
      upstreamClb_(ShmTransport::makeWriter(layout_.upstream));
    }
    if (downstreamClb_) {
      downstreamClb_(ShmTransport::makeReader(layout_.downstream, ErrorBus{bus_.systemBus}));
    }
  }

private:
  ClientBus &bus_;
  ShmLayout &layout_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  AtomicBool running_{false};
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP