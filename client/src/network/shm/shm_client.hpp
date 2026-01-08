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
#include "network/transport/shm/shm_layout.hpp"
#include "network/transport/shm/shm_reactor.hpp"
#include "network/transport/shm/shm_transport.hpp"
#include "traits.hpp"
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
      : bus_{bus}, name_{Config::get<String>("shm.shm_name")},
        size_{Config::get<size_t>("shm.shm_size")},
        reactor_{init(), ReactorType::Client, bus_.systemBus} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) {}

  void start() {
    workerThread_ = std::jthread([this]() {
      try {
        running_ = true;

        utils::setThreadRealTime();
        if (ClientConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ClientConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_INFO_SYSTEM("Network thread started on the core {}", coreId);
        } else {
          LOG_INFO_SYSTEM("Network thread started");
        }
        notifyConnected();
        reactor_.run();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        stop();
        bus_.post(InternalError(StatusCode::Error, e.what()));
      } catch (...) {
        LOG_ERROR_SYSTEM("unknown exception in CtxRunner");
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

      if (workerThread_.joinable()) {
        workerThread_.join();
      }

      if (layout_) {
        munmap(layout_, size_);
        layout_ = nullptr;
      }
    } catch (const std::exception &e) {
      bus_.post(InternalError(StatusCode::Error, e.what()));
    } catch (...) {
      bus_.post(InternalError(StatusCode::Error, "unknown exception in ShmClient"));
    }
  }

private:
  ShmLayout *init() {
    void *addr = utils::mapSharedMemory(name_, size_, false);

    utils::warmMemory(addr, size_, false);
    utils::lockMemory(addr, size_);

    layout_ = static_cast<ShmLayout *>(addr);

    if (layout_->downstreamFtx.load(std::memory_order_acquire) == 0) {
      throw std::runtime_error("Server is not ready or SHM is uninitialized");
    }

    layout_->upstreamFtx.store(1, std::memory_order_release);
    utils::futexWake(layout_->upstreamFtx);

    LOG_INFO_SYSTEM("Client successfully attached to SHM: {}", name_);
    return layout_;
  }

  void notifyConnected() {
    if (upstreamClb_) {
      LOG_INFO_SYSTEM("Connected upstream");
      upstreamClb_(ShmTransport(ShmTransportType::Upstream, layout_->upstream, reactor_));
    }
    if (downstreamClb_) {
      LOG_INFO_SYSTEM("Connected downstream");
      downstreamClb_(ShmTransport(ShmTransportType::Downstream, layout_->downstream, reactor_));
    }
  }

private:
  ClientBus &bus_;

  const String name_;
  const size_t size_;

  ShmLayout *layout_;

  ShmReactor<ShmTransport> reactor_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  std::atomic_bool running_{false};
  std::jthread workerThread_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP