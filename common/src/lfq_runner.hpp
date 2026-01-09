/**
 * @author Vladimir Pavliv
 * @date 2025-12-29
 */

#ifndef HFT_COMMON_LFQRUNNER_HPP
#define HFT_COMMON_LFQRUNNER_HPP

#include <memory>

#include <atomic>
// #include <boost/lockfree/spsc_queue.hpp>
// #include <folly/ProducerConsumerQueue.h>
// #include <rigtorp/SPSCQueue.h>
#include <thread>

#include "bus/system_bus.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/sync_utils.hpp"
#include "vyukov_queue.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageType, typename Consumer, size_t Capacity = 65536>
class LfqRunner {
  using Queue = VyukovQueue<MessageType, Capacity>; // <= 10.2 ns
  // using Queue = rigtorp::SPSCQueue<MessageType>; // <= 15.6 ns
  // using Queue = boost::lockfree::spsc_queue<MessageType>; // <= 36.9 ns
  // using Queue = folly::ProducerConsumerQueue<MessageType>; // <= 38.3 ns

public:
  LfqRunner(Consumer &consumer, ErrorBus &&bus) : consumer_{consumer}, bus_{std::move(bus)} {}

  LfqRunner(CoreId id, Consumer &consumer, ErrorBus &&bus)
      : coreId_{id}, consumer_{consumer}, bus_{std::move(bus)} {}

  ~LfqRunner() {}

  void run() {
    thread_ = std::jthread([this]() {
      try {
        running_.store(true);
        running_.notify_all();

        utils::setThreadRealTime();
        if (coreId_.has_value()) {
          LOG_INFO("LfqRunner started on core {}", coreId_.value());
          utils::pinThreadToCore(coreId_.value());
        } else {
          LOG_INFO("LfqRunner started");
        }

        uint64_t cycles = 0;
        MessageType message;
        while (running_.load(std::memory_order_acquire)) {
          if (queue_.pop(message)) {
            cycles = 0;
            do {
              consumer_.post(message);
            } while (queue_.pop(message));
          } else {
            asm volatile("pause" ::: "memory");
            continue;
            if (++cycles > BUSY_WAIT_CYCLES && running_.load(std::memory_order_acquire)) {
              const auto ftxVal = ftx_.load(std::memory_order_acquire);
              sleeping_.store(true, std::memory_order_release);

              if (queue_.pop(message)) {
                sleeping_.store(false, std::memory_order_release);
                consumer_.post(message);
                cycles = 0;
                continue;
              }

              utils::futexWait(ftx_, ftxVal);
              sleeping_.store(false, std::memory_order_release);
              cycles = 0;
            }
          }
        }
      } catch (const std::exception &ex) {
        LOG_ERROR_SYSTEM("{}", ex.what());
        bus_.post(InternalError{StatusCode::Error, ex.what()});
      }
    });
    running_.wait(false);
  }

  void stop() {
    running_.store(false, std::memory_order_release);
    ftx_.fetch_add(1, std::memory_order_release);
    utils::futexWake(ftx_);
  }

  inline bool post(CRef<MessageType> message) {
    size_t waitCycles = 0;
    while (!queue_.push(message)) {
      if (++waitCycles > BUSY_WAIT_CYCLES) {
        return false;
      }
      asm volatile("pause" ::: "memory");
    }
    if (sleeping_.load(std::memory_order_acquire)) [[unlikely]] {
      ftx_.fetch_add(1, std::memory_order_release);
      utils::futexWake(ftx_);
      sleeping_.store(false, std::memory_order_release);
    }
    return true;
  }

private:
  const Optional<CoreId> coreId_;

  alignas(64) Queue queue_;
  alignas(64) AtomicBool running_{false};
  alignas(64) AtomicBool sleeping_{false};
  alignas(64) AtomicUInt32 ftx_;

  alignas(64) Consumer &consumer_;
  ErrorBus bus_;
  std::jthread thread_;
};

} // namespace hft

#endif // HFT_COMMON_CTXRUNNER_HPP
