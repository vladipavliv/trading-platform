/**
 * @author Vladimir Pavliv
 * @date 2025-12-29
 */

#ifndef HFT_COMMON_LFQRUNNER_HPP
#define HFT_COMMON_LFQRUNNER_HPP

#include <memory>

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <folly/ProducerConsumerQueue.h>
#include <thread>

#include "boost_types.hpp"
#include "bus/system_bus.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/utils.hpp"
#include "vyukov_queue.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageType, typename Consumer, size_t Capacity = 65536>
class alignas(64) LfqRunner {
  static constexpr size_t MAX_EMPTY_CYCLES = 1'000'000;

  // using Queue = boost::lockfree::spsc_queue<MessageType>;
  using Queue = VyukovQueue<MessageType, Capacity>;
  // using Queue = folly::ProducerConsumerQueue<MessageType>;

public:
  LfqRunner(Consumer &consumer, ErrorBus &&bus) : consumer_{consumer} {}

  LfqRunner(CoreId id, Consumer &consumer, ErrorBus &&bus) : coreId_{id}, consumer_{consumer} {}

  ~LfqRunner() {}

  void run() {
    thread_ = Thread([this]() {
      if (coreId_.has_value()) {
        utils::pinThreadToCore(coreId_.value());
      }

      running_.store(true);
      running_.notify_all();

      MessageType message;
      // size_t idleCycles = 0;
      while (running_.load(std::memory_order_acquire)) {
        if (queue_.pop(message)) {
          do {
            consumer_.post(message);
          } while (queue_.pop(message));
        } else {
          asm volatile("pause" ::: "memory");
        }
      }
    });
    running_.wait(false);
  }

  void stop() { running_.store(false, std::memory_order_release); }

  inline bool post(CRef<MessageType> message) {
    size_t waitCycles = 0;
    while (!queue_.push(message)) {
      if (++waitCycles > MAX_EMPTY_CYCLES) {
        return false;
      }
      asm volatile("pause" ::: "memory");
    }
    return true;
  }

private:
  const Optional<CoreId> coreId_;

  Queue queue_;
  std::atomic_bool running_{false};

  Consumer &consumer_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_CTXRUNNER_HPP
