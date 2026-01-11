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
#include "transport/shm/shm_reactor.hpp"
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

  explicit ShmClient(ClientBus &bus)
      : bus_{bus}, layout_{ShmManager::layout()},
        reactor_{layout_, ReactorType::Client, bus_.systemBus} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) {}

  void start() {
    if (running_) {
      LOG_ERROR_SYSTEM("ShmClient is already running");
      return;
    }
    workerThread_ = std::jthread([this]() {
      try {
        running_ = true;

        utils::setThreadRealTime();
        if (ClientConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ClientConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_INFO_SYSTEM("Communication thread started on the core {}", coreId);
        } else {
          LOG_INFO_SYSTEM("Communication thread started");
        }

        layout_.upstreamFtx.store(1, std::memory_order_release);
        utils::futexWake(layout_.upstreamFtx);

        notifyConnected();
        reactor_.run();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        stop();
        bus_.post(InternalError(StatusCode::Error, e.what()));
      } catch (...) {
        LOG_ERROR_SYSTEM("unknown exception in ShmClient");
        stop();
        bus_.post(InternalError(StatusCode::Error, "unknown exception in ShmClient"));
      }
    });
  }

  void stop() {
    LOG_INFO("ShmClient stop");
    try {
      if (!running_) {
        return;
      }
      running_ = false;
      reactor_.stop();
    } catch (const std::exception &e) {
      bus_.post(InternalError(StatusCode::Error, e.what()));
    } catch (...) {
      bus_.post(InternalError(StatusCode::Error, "unknown exception in ShmClient"));
    }
  }

private:
  void notifyConnected() {
    if (upstreamClb_) {
      LOG_INFO_SYSTEM("Connected upstream");
      upstreamClb_(ShmTransport(ShmTransportType::Upstream, layout_.upstream, reactor_));
    }
    if (downstreamClb_) {
      LOG_INFO_SYSTEM("Connected downstream");
      downstreamClb_(ShmTransport(ShmTransportType::Downstream, layout_.downstream, reactor_));
    }
  }

private:
  ClientBus &bus_;
  ShmLayout &layout_;

  ShmReactor<ShmTransport> reactor_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  std::atomic_bool running_{false};
  std::jthread workerThread_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP