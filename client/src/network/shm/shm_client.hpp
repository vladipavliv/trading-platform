/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_CLIENT_SHMCLIENT_HPP
#define HFT_CLIENT_SHMCLIENT_HPP

#include <functional>
#include <memory>
#include <string>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include "config/client_config.hpp"
#include "network/transport/shm/shm_layout.hpp"
#include "network/transport/shm/shm_reactor.hpp"
#include "network/transport/shm/shm_transport.hpp"
#include "traits.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Client that creates and manages shared memory
 */
class ShmClient {
public:
  using StreamClb = std::function<void(ShmTransport &&transport)>;
  using DatagramClb = std::function<void(ShmTransport &&transport)>;

  explicit ShmClient(ClientBus &bus)
      : bus_{bus}, name_{Config::get<String>("shm.shm_name")},
        size_{Config::get<size_t>("shm.shm_size")}, reactor_{init(), ReactorType::Client} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    workerThread_ = Thread([this]() {
      try {
        utils::setTheadRealTime();
        if (ClientConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ClientConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_DEBUG("Network thread started on the core {}", coreId);
        } else {
          LOG_DEBUG("Network thread started");
        }
        notifyConnected();

        bus_.post(LoginResponse{0, true, ""});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        bus_.post(LoginResponse{0, true, ""});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));

        running_ = true;
        reactor_.run();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        bus_.post(InternalError(StatusCode::Error, e.what()));
      }
    });
  }

  void stop() {
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
    LOG_INFO_SYSTEM("ShmClient shutdown");
  }

private:
  ShmLayout *init() {
    int fd = open(name_.c_str(), O_RDWR);
    if (fd == -1) {
      throw std::runtime_error("Failed to open SHM. Is the server running?");
    }

    void *addr = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (addr == MAP_FAILED) {
      throw std::runtime_error("Hugepage mmap failed on client");
    }

    // Warm up the client's process page table
    volatile uint8_t *p = static_cast<volatile uint8_t *>(addr);
    for (size_t i = 0; i < size_; i += 4096) {
      uint8_t dummy = p[i];
      (void)dummy;
    }

    layout_ = static_cast<ShmLayout *>(addr);

    if (mlock(addr, size_) != 0) {
      LOG_ERROR_SYSTEM("Could not mlock SHM on client.");
    }

    if (layout_->downstreamFtx.load(std::memory_order_acquire) == 0) {
      throw std::runtime_error("Server is down");
    }

    layout_->upstreamFtx.store(1, std::memory_order_release);
    utils::futexWake(layout_->upstreamFtx);

    LOG_INFO_SYSTEM("Client attached to SHM: {}", name_);
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

    if (datagramClb_) {
      LOG_INFO_SYSTEM("Connected datagram");
      datagramClb_(ShmTransport(ShmTransportType::Datagram, layout_->broadcast, reactor_));
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
  Thread workerThread_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP