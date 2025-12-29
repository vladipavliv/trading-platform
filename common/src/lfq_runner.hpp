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
template <typename MessageType, typename Consumer>
class alignas(64) LfqRunner {
  static constexpr size_t MAX_EMPTY_CYCLES = 1'000'000;
  static constexpr size_t CAPACITY = 2'097'152;

  // using Queue = boost::lockfree::spsc_queue<MessageType>;
  // using Queue = VyukovQueue<MessageType, CAPACITY>;
  using Queue = folly::ProducerConsumerQueue<MessageType>;

public:
  LfqRunner(Consumer &consumer, ErrorBus &&bus) : queue_{CAPACITY}, consumer_{consumer} {}

  LfqRunner(CoreId id, Consumer &consumer, ErrorBus &&bus)
      : coreId_{id}, queue_{CAPACITY}, consumer_{consumer} {}

  ~LfqRunner() {}

  void run() {
    thread_ = Thread([this]() {
      if (coreId_.has_value()) {
        utils::pinThreadToCore(coreId_.value());
      }

      running_.store(true);
      running_.notify_all();

      MessageType message;
      size_t idleCycles = 0;
      while (running_.load(std::memory_order_acquire)) {
        if (queue_.read(message)) {
          consumer_.post(message);
        } else {
          if (++idleCycles < MAX_EMPTY_CYCLES) {
            asm volatile("pause" ::: "memory");
          } else {
            std::this_thread::yield();
            idleCycles = 0;
          }
        }
      }
    });
    running_.wait(false);
  }

  void stop() { running_.store(false, std::memory_order_release); }

  bool post(CRef<MessageType> message) {
    size_t waitCycles = 0;
    while (!queue_.write(message)) {
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
