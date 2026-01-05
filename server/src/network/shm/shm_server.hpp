/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_SERVER_SHARED_MEMORY_SERVER_HPP
#define HFT_SERVER_SHARED_MEMORY_SERVER_HPP

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

  ShmServer()
      : name_{Config::get<String>("shm.shm_name")}, size_{Config::get<size_t>("shm.shm_size")},
        reactor_{init()} {}

  ~ShmServer() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void run() {}

  void stop() {
    if (layout_) {
      layout_->clientConnected.store(false);
      layout_->serverReady.store(false);

      shm_->destroy<ShmLayout>("Layout");
      layout_ = nullptr;
    }

    if (shm_) {
      shm_.reset();
      boost::interprocess::shared_memory_object::remove(name_.c_str());
      LOG_INFO_SYSTEM("Shared memory cleaned up");
    }
  }

  size_t poll() {
    if (!running_ && layout_->clientConnected.load(std::memory_order_acquire)) {
      notifyClientConnected();
      running_ = true;
    }
    if (running_ && !layout_->clientConnected.load(std::memory_order_acquire)) {
      running_ = false;
    }

    return 0;
  }

private:
  ShmLayout *init() {
    boost::interprocess::shared_memory_object::remove(name_.c_str());

    shm_.reset(new SharedMemory(boost::interprocess::create_only, name_.c_str(), size_));
    if (!shm_) {
      throw std::runtime_error("Failed to construct shared memory object");
    }

    layout_ = shm_->construct<ShmLayout>("Layout")();
    if (!layout_) {
      throw std::runtime_error("Failed to construct shared memory layout");
    }

    layout_->serverReady.store(true, std::memory_order_release);
    LOG_INFO_SYSTEM("Shared memory initialized: {}", name_);

    return layout_;
  }

  void notifyClientConnected() {
    if (upstreamClb_) {
      upstreamClb_(ShmTransport(layout_->upstream, reactor_));
    }
    if (downstreamClb_) {
      downstreamClb_(ShmTransport(layout_->downstream, reactor_));
    }
    if (datagramClb_) {
      datagramClb_(ShmTransport(layout_->broadcast, reactor_));
    }
  }

private:
  const String name_;
  const size_t size_;

  UPtr<SharedMemory> shm_;
  ShmLayout *layout_;

  ShmReactor<ShmTransport> reactor_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  bool running_{false};
};
} // namespace hft::server

#endif // HFT_SERVER_SHARED_MEMORY_SERVER_HPP