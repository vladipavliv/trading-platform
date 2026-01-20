/**
 * @author Vladimir Pavliv
 * @date 2026-01-15
 */

#include "shm_reactor.hpp"
#include "config/config.hpp"
#include "logging.hpp"
#include "shm_reader.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

ShmReactor::ShmReactor(ErrorBus &&bus) : bus_{std::move(bus)} {
  LOG_INFO_SYSTEM("ShmReactor ctor");
  ShmReactor *expected = nullptr;
  if (!instance.compare_exchange_strong(expected, this, std::memory_order_release)) {
    throw std::runtime_error("ShmReactor is already initialized");
  }
}

ShmReactor::~ShmReactor() { LOG_DEBUG_SYSTEM("~ShmReactor"); }

void ShmReactor::add(ShmReader *reader) {
  if (running_.load(std::memory_order_acquire)) {
    LOG_ERROR("ShmReactor is already running");
    return;
  }
  LOG_DEBUG("Register ShmReader");
  readers_.push_back(reader);
}

void ShmReactor::run() {
  if (running_.load()) {
    LOG_ERROR_SYSTEM("ShmReactor is already running");
    return;
  }
  running_.store(true, std::memory_order_release);
  LOG_DEBUG("ShmReactor run");
  thread_ = std::jthread([this]() {
    try {
      utils::setThreadRealTime();
      const auto coreId = Config::get_optional<CoreId>("cpu.core_network");
      if (coreId.has_value()) {
        LOG_DEBUG("Pin ShmReader thread to core {}", *coreId);
        utils::pinThreadToCore(*coreId);
      }
      loop();
      LOG_DEBUG("ShmReactor::loop end");
    } catch (const std::exception &ex) {
      bus_.post(InternalError{StatusCode::Error, String("Exception in ShmReader {}") + ex.what()});
    } catch (...) {
      bus_.post(InternalError{StatusCode::Error, "Unknown exception in ShmReader"});
    }
    running_.store(false, std::memory_order_release);
  });
}

void ShmReactor::stop() {
  LOG_DEBUG_SYSTEM("ShmReactor stop");
  if (!running_.load(std::memory_order_acquire)) {
    LOG_WARN_SYSTEM("already stopped");
    return;
  }
  running_.store(false, std::memory_order_release);
  for (auto *rdr : readers_) {
    rdr->notify();
  }
  readers_.clear();
  instance.store(nullptr, std::memory_order_release);
  LOG_DEBUG("Joining ShmReactor thread");
  utils::join(thread_);
  LOG_DEBUG("ShmReactor stopped");
}

void ShmReactor::loop() {
  LOG_DEBUG_SYSTEM("ShmReactor::loop start {} readers", readers_.size());
  if (readers_.empty()) {
    LOG_ERROR_SYSTEM("No ShmReaders registered.");
    return;
  }
  SpinWait waiter{SPIN_RETRIES_WARM};
  while (running_.load(std::memory_order_acquire)) {
    bool busy = false;
    for (size_t i = 0; i < readers_.size(); ++i) {
      auto res = readers_[i]->poll();
      if (res == ShmReader::PollResult::Vanished) {
        LOG_DEBUG_SYSTEM("Reader vanished");
        readers_[i] = readers_.back();
        readers_.pop_back();
        if (readers_.empty()) {
          LOG_DEBUG_SYSTEM("No more active readers");
          return;
        }
        --i;
        continue;
      } else if (res == ShmReader::PollResult::Busy) {
        busy = true;
      }
    }
    if (busy) {
      waiter.reset();
    } else if (!++waiter) {
      readers_[0]->wait();
      waiter.reset();
    }
  }
}

bool ShmReactor::running() const { return running_.load(std::memory_order_acquire); }

} // namespace hft
