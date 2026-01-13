/**
 * @author Vladimir Pavliv
 * @date 2025-12-29
 */

#ifndef HFT_COMMON_LFQRUNNER_HPP
#define HFT_COMMON_LFQRUNNER_HPP

#include <memory>
#include <thread>

#include "bus/system_bus.hpp"
#include "container_types.hpp"
#include "containers/sequenced_spsc.hpp"
#include "containers/vyukov_mpmc.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageType, typename Consumer, size_t Capacity = 65536>
class LfqRunner {
  using Queue = SequencedSPSC<Capacity>;

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

        lfqLoop();
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
    auto msgPtr = reinterpret_cast<const uint8_t *>(&message);
    auto msgSize = sizeof(MessageType);
    size_t waitCycles = 0;
    while (!queue_.write(msgPtr, msgSize)) {
      if (++waitCycles > BUSY_WAIT_CYCLES) {
        LOG_ERROR_SYSTEM("Failed to push to LfqRunner");
        return false;
      }
      asm volatile("pause" ::: "memory");
    }
    if (sleeping_.load(std::memory_order_seq_cst)) [[unlikely]] {
      ftx_.store(++wakeCounter_, std::memory_order_release);
      utils::futexWake(ftx_);
    }
    return true;
  }

private:
  void lfqLoop() {
    uint64_t cycles = 0;
    MessageType message;
    auto msgPtr = reinterpret_cast<uint8_t *>(&message);
    auto msgSize = sizeof(MessageType);
    while (running_.load(std::memory_order_acquire)) {
      if (queue_.read(msgPtr, msgSize)) {
        cycles = 0;
        do {
          consumer_.post(message);
        } while (queue_.read(msgPtr, msgSize));
        continue;
      }
      asm volatile("pause" ::: "memory");
      if (++cycles < BUSY_WAIT_CYCLES) {
        continue;
      }
      const auto ftxVal = ftx_.load(std::memory_order_acquire);
      sleeping_.store(true, std::memory_order_seq_cst);

      if (queue_.read(msgPtr, msgSize)) {
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

private:
  const Optional<CoreId> coreId_;

  alignas(64) Queue queue_;
  alignas(64) AtomicBool running_{false};
  alignas(64) AtomicBool sleeping_{false};
  alignas(64) AtomicUInt32 ftx_{0};
  alignas(64) uint64_t wakeCounter_{0};

  alignas(64) Consumer &consumer_;
  ErrorBus bus_;
  std::jthread thread_;
};

} // namespace hft

#endif // HFT_COMMON_LFQRUNNER_HPP
