/**
 * @author Vladimir Pavliv
 * @date 2026-01-04
 */

#ifndef HFT_CLIENT_SHMCLIENT_HPP
#define HFT_CLIENT_SHMCLIENT_HPP

#include <boost/interprocess/managed_shared_memory.hpp>

#include <functional>
#include <memory>
#include <string>

#include "config/client_config.hpp"
#include "network/transport/shm/shm_layout.hpp"
#include "network/transport/shm/shm_read_ctx.hpp"
#include "network/transport/shm/shm_transport.hpp"
#include "utils/utils.hpp"

namespace hft::client {

/**
 * @brief Client that creates and manages shared memory
 */
class ShmClient {
  using SharedMemory = boost::interprocess::managed_shared_memory;

public:
  using StreamTransport = ShmTransport;
  using DatagramTransport = ShmTransport;

  using StreamClb = std::function<void(StreamTransport &&transport)>;
  using DatagramClb = std::function<void(DatagramTransport &&transport)>;

  ShmClient()
      : name_{Config::get<String>("shm.shm_name")}, size_{Config::get<size_t>("shm.shm_size")} {}

  ~ShmClient() { stop(); }

  void setUpstreamClb(StreamClb &&streamClb) { upstreamClb_ = std::move(streamClb); }

  void setDownstreamClb(StreamClb &&streamClb) { downstreamClb_ = std::move(streamClb); }

  void setDatagramClb(DatagramClb &&datagramClb) { datagramClb_ = std::move(datagramClb); }

  void start() {
    shm_.reset(new SharedMemory(boost::interprocess::open_only, name_.c_str()));
    if (!shm_) {
      throw std::runtime_error("Failed to construct shared memory object");
    }

    auto [ptr, size] = shm_->find<ShmLayout>("Layout");
    if (ptr == nullptr) {
      throw std::runtime_error("Failed to construct shared memory layout");
    }

    layout_ = ptr;
    if (!layout_->serverReady.load(std::memory_order_acquire)) {
      throw std::runtime_error("Server is down");
    }
    layout_->clientConnected.store(true, std::memory_order_release);

    workerThread_ = Thread([this]() {
      try {
        running_ = true;
        utils::setTheadRealTime();
        if (ClientConfig::cfg.coreNetwork.has_value()) {
          const auto coreId = *ClientConfig::cfg.coreNetwork;
          utils::pinThreadToCore(coreId);
          LOG_DEBUG("Network thread started on the core {}", coreId);
        } else {
          LOG_DEBUG("Network thread started");
        }
        pollLoop();
      } catch (const std::exception &e) {
        LOG_ERROR_SYSTEM("Exception in network thread {}", e.what());
        stop();
      }
    });
    LOG_INFO_SYSTEM("Shared memory initialized: {}", name_);
  }

  void connect() {
    LOG_INFO_SYSTEM("Connecting to the server");
    if (upstreamClb_) {
      LOG_INFO_SYSTEM("Connected upstream");
      upstreamClb_(ShmTransport(layout_->upstream, upstreamRdCtx_));
    }

    if (downstreamClb_) {
      LOG_INFO_SYSTEM("Connected downstream");
      downstreamClb_(ShmTransport(layout_->downstream, downstreamRdCtx_));
    }

    if (datagramClb_) {
      LOG_INFO_SYSTEM("Connected datagram");
      datagramClb_(ShmTransport(layout_->broadcast, datagramRdCtx_));
    }
  }

  void stop() { running_ = false; }

private:
  void pollLoop() {
    while (running_) {
      size_t bytesProcessed = 0;
      if (upstreamRdCtx_) {
        auto ctx = upstreamRdCtx_;
        upstreamRdCtx_.reset();
        size_t bytesRead = layout_->upstream.read(ctx.dest.data(), ctx.dest.size());
        if (bytesRead != 0) {
          bytesProcessed += bytesRead;
          ctx.clb(IoResult::Ok, bytesRead);
        } else {
          upstreamRdCtx_ = ctx;
        }
      }
      if (downstreamRdCtx_) {
        auto ctx = downstreamRdCtx_;
        downstreamRdCtx_.reset();
        size_t bytesRead = layout_->downstream.read(ctx.dest.data(), ctx.dest.size());
        if (bytesRead != 0) {
          bytesProcessed += bytesRead;
          ctx.clb(IoResult::Ok, bytesRead);
        } else {
          downstreamRdCtx_ = ctx;
        }
      }
      if (datagramRdCtx_) {
        auto ctx = datagramRdCtx_;
        datagramRdCtx_.reset();
        size_t bytesRead = layout_->broadcast.read(ctx.dest.data(), ctx.dest.size());
        if (bytesRead != 0) {
          bytesProcessed += bytesRead;
          ctx.clb(IoResult::Ok, bytesRead);
        } else {
          datagramRdCtx_ = ctx;
        }
      }
      if (bytesProcessed == 0) {
        asm volatile("pause" ::: "memory");
      }
    }
    if (layout_) {
      layout_->clientConnected.store(false, std::memory_order_release);
      layout_ = nullptr;
    }
    shm_.reset();
  }

private:
  const String name_;
  const size_t size_;

  UPtr<SharedMemory> shm_;
  ShmLayout *layout_;

  ShmReadCtx upstreamRdCtx_;
  ShmReadCtx downstreamRdCtx_;
  ShmReadCtx datagramRdCtx_;

  StreamClb upstreamClb_;
  StreamClb downstreamClb_;
  DatagramClb datagramClb_;

  std::atomic_bool running_{false};
  Thread workerThread_;
};
} // namespace hft::client

#endif // HFT_CLIENT_SHMCLIENT_HPP