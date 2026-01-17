/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#include "shm_reactor.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "shm_reader.hpp"

namespace hft {

void ShmReactor::add(ShmReader *reader) {
  if (running_.load(std::memory_order_acquire)) {
    throw std::runtime_error("ShmReactor::add is called after start");
  }
  readers_.push_back(reader);
}

void ShmReactor::run() {
  LOG_DEBUG_SYSTEM("ShmReactor run");
  thread_ = std::jthread([this]() {
    try {
      utils::setThreadRealTime();
      const auto coreId = Config::get_optional<CoreId>("cpu.core_network");
      if (coreId.has_value()) {
        LOG_DEBUG_SYSTEM("Pin ShmReader thread to core {}", *coreId);
        utils::pinThreadToCore(*coreId);
      }
      running_.store(true, std::memory_order_release);
      loop();
      LOG_DEBUG_SYSTEM("ShmReactor::loop end");
    } catch (const std::exception &ex) {
      LOG_ERROR_SYSTEM("Exception in ShmReader {}", ex.what());
      running_.store(false, std::memory_order_release);
    } catch (...) {
      LOG_ERROR_SYSTEM("Unknown exception in ShmReader");
      running_.store(false, std::memory_order_release);
    }
  });
}

void ShmReactor::stop() {
  if (!running_.load(std::memory_order_acquire)) {
    return;
  }
  LOG_DEBUG_SYSTEM("ShmReactor stop");
  running_.store(false, std::memory_order_release);
  for (auto *rdr : readers_) {
    rdr->notify();
  }
  if (thread_.joinable()) {
    thread_.join();
  }
}

void ShmReactor::loop() {
  LOG_DEBUG_SYSTEM("ShmReactor::loop start {}", readers_.size());
  SpinWait waiter;
  while (running_.load(std::memory_order_acquire)) {
    bool busy = false;
    for (auto *reader : readers_) {
      busy |= reader->poll();
    }
    if (busy) {
      waiter.reset();
    } else if (!++waiter) {
      // TODO(self): fix when there is more then 1 reader
      readers_.front()->wait();
    }
  }
}

bool ShmReactor::running() const { return running_.load(std::memory_order_acquire); }

} // namespace hft
