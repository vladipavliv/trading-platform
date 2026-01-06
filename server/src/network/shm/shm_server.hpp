/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_SERVER_SHMSERVER_HPP
#define HFT_SERVER_SHMSERVER_HPP

#include <boost/interprocess/managed_shared_memory.hpp>

#include <functional>
#include <memory>
#include <string>

#include "config/server_config.hpp"
#include "network/async_transport.hpp"
#include "network/transport/shm/shm_layout.hpp"
#include "network/transport/shm/shm_reactor.hpp"
#include "network/transport/shm/shm_transport.hpp"
#include "utils/utils.hpp"

namespace hft::server {

/**
 * @brief
 */
class ShmServer {
  using SharedMemory = boost::interprocess::managed_shared_memory;

public:
  using StreamTransport = ShmTransport;
  using DatagramTransport = ShmTransport;

  using StreamClb = std::function<void(StreamTransport &&transport)>;
  using DatagramClb = std::function<void(DatagramTransport &&transport)>;

  explicit ShmServer(ServerBus &bus)
      : bus_{bus}, name_{Config::get<String>("shm.shm_name")},
        size_{Config::get<size_t>("shm.shm_size")}, reactor_{init(), ReactorType::Server} {}

  ~ShmServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    workerThread_ = Thread([this]() {
      try {
        utils::setTheadRealTime();
        if (ServerConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ServerConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_DEBUG("Network thread started on the core {}", coreId);
        } else {
          LOG_DEBUG("Network thread started");
        }

        waitForConnection();
        notifyClientConnected();

        bus_.post(ServerLoginRequest{0, {"client0", "password0"}});
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        bus_.post(ServerTokenBindRequest{1, {0}});
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
      unlink(name_.c_str());
      layout_ = nullptr;
    }
    LOG_INFO_SYSTEM("ShmServer shutdown");
  }

  auto getHook() -> std::function<void(Callback &&clb)> {
    // execute right away on the same thread
    return [](Callback &&clb) { clb(); };
  }

private:
  void waitForConnection() {
    LOG_INFO_SYSTEM("Waiting for client connection");
    size_t clientConnected = layout_->upstreamFtx.load(std::memory_order_acquire);
    while (clientConnected == 0) {
      utils::futexWait(layout_->upstreamFtx, clientConnected);
      clientConnected = layout_->upstreamFtx.load(std::memory_order_acquire);
    }
    LOG_INFO_SYSTEM("Client connected");
  }

private:
  ShmLayout *init() {
    // manual mm open for huge pages
    unlink(name_.c_str());

    int fd = open(name_.c_str(), O_CREAT | O_RDWR | O_EXCL, 0666);
    if (ftruncate(fd, size_) == -1) {
      close(fd);
      throw std::runtime_error("Failed to ftruncate SHM file: " + std::string(strerror(errno)));
    }

    void *addr = mmap(NULL, size_, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if (addr == MAP_FAILED) {
      throw std::runtime_error("Hugepage mmap failed");
    }
    volatile uint8_t *p = static_cast<volatile uint8_t *>(addr);
    for (size_t i = 0; i < size_; i += 4096) {
      p[i] = 0;
    }

    layout_ = new (addr) ShmLayout();
    if (!layout_) {
      throw std::runtime_error("Failed to construct shared memory layout");
    }

    if (mlock(addr, size_) != 0) {
      LOG_ERROR_SYSTEM("mlock failed: {} (errno: {})", strerror(errno), errno);
    }

    layout_->downstreamFtx.store(1, std::memory_order_release);
    utils::futexWake(layout_->downstreamFtx);

    LOG_INFO_SYSTEM("Shared memory initialized: {}", name_);
    return layout_;
  }

  void notifyClientConnected() {
    if (upstreamClb_) {
      upstreamClb_(ShmTransport(ShmTransportType::Upstream, layout_->upstream, reactor_));
    }
    if (downstreamClb_) {
      downstreamClb_(ShmTransport(ShmTransportType::Downstream, layout_->downstream, reactor_));
    }
    if (datagramClb_) {
      datagramClb_(ShmTransport(ShmTransportType::Datagram, layout_->broadcast, reactor_));
    }
  }

private:
  ServerBus &bus_;

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
} // namespace hft::server

#endif // HFT_SERVER_SHMSERVER_HPP