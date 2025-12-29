/**
 * @author Vladimir Pavliv
 * @date 2025-12-29
 */

#ifndef HFT_COMMON_LFQRUNNER_HPP
#define HFT_COMMON_LFQRUNNER_HPP

#include <memory>

#include <atomic>
#include <boost/lockfree/spsc_queue.hpp>
#include <thread>

#include "boost_types.hpp"
#include "bus/system_bus.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageType, typename Matcher>
class LfqRunner {
  using Queue = boost::lockfree::spsc_queue<MessageType>;
  const size_t MAX_EMPTY_CYCLES = 10'000'000;

public:
  LfqRunner(Matcher &matcher, ErrorBus &&bus) : queue_{1000000}, matcher_{matcher} {}

  LfqRunner(CoreId id, Matcher &matcher, ErrorBus &&bus)
      : coreId_{id}, queue_{1000000}, matcher_{matcher} {}

  ~LfqRunner() {}

  void run() {
    running_ = true;
    thread_ = Thread([this]() {
      if (coreId_.has_value()) {
        utils::pinThreadToCore(coreId_.value());
      }

      MessageType message;
      size_t idleCycles = 0;
      while (running_.load(std::memory_order_relaxed)) {
        if (queue_.pop(message)) {
          matcher_.process(message);
        } else {
          if (++idleCycles < MAX_EMPTY_CYCLES) {
            asm volatile("pause" ::: "memory");
          } else {
            // std::this_thread::yield();
            idleCycles = 0;
          }
        }
      }
    });
  }

  void stop() { running_ = false; }

  void post(CRef<MessageType> message) {
    size_t waitCycles = 0;
    while (!queue_.push(message)) {
      if (++waitCycles > MAX_EMPTY_CYCLES) {
        throw std::runtime_error("failed to post to lfq runner");
      }
      asm volatile("pause" ::: "memory");
    }
  }

private:
  const Optional<CoreId> coreId_;

  Queue queue_;
  std::atomic_bool running_{false};

  Matcher &matcher_;
  Thread thread_;
};

} // namespace hft

#endif // HFT_COMMON_CTXRUNNER_HPP
