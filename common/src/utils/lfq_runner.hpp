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
#include "gateway/internal_order.hpp"
#include "internal_error.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"
#include "types/functional_types.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageT, typename ConsumerT, typename BusT, size_t Capacity = 65536>
class LfqRunner {
  using Queue = SequencedSPSC<Capacity>;

public:
  LfqRunner(ConsumerT &consumer, BusT &bus, Optional<CoreId> coreId = std::nullopt,
            bool feedFromBus = false)
      : consumer_{consumer}, bus_{bus}, coreId_{coreId} {
    if (feedFromBus) {
      using SelfT = LfqRunner<MessageT, ConsumerT, BusT, Capacity>;
      bus_.template subscribe<MessageT>(
          CRefHandler<MessageT>::template bind<SelfT, &SelfT::post>(this));
    }
  }

  ~LfqRunner() { stop(); }

  void run() {
    LOG_DEBUG("LfqRunner run");
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
      LOG_DEBUG("LfqRunner finished");
    });
    running_.wait(false);
  }

  void stop() {
    LOG_DEBUG("LfqRunner stop");
    running_.store(false, std::memory_order_release);
    ftx_.fetch_add(1, std::memory_order_release);
    utils::futexWake(ftx_);
  }

  inline void post(CRef<MessageT> message) {
    LOG_DEBUG("{}", toString(message));
    auto msgPtr = reinterpret_cast<const uint8_t *>(&message);
    auto msgSize = sizeof(MessageT);

    SpinWait waiter;
    while (!queue_.write(msgPtr, msgSize)) {
      if (!++waiter) {
        break;
      }
    }
    if (sleeping_.load(std::memory_order_seq_cst)) [[unlikely]] {
      ftx_.store(++wakeCounter_, std::memory_order_release);
      utils::futexWake(ftx_);
    }
  }

private:
  void lfqLoop() {
    MessageT message;
    auto msgPtr = reinterpret_cast<uint8_t *>(&message);
    auto msgSize = sizeof(MessageT);

    SpinWait waiter;
    while (running_.load(std::memory_order_acquire)) {
      if (queue_.read(msgPtr, msgSize)) {
        waiter.reset();
        do {
          consumer_.post(message);
        } while (queue_.read(msgPtr, msgSize));
        continue;
      }
      if (++waiter) {
        continue;
      }
      const auto ftxVal = ftx_.load(std::memory_order_acquire);
      sleeping_.store(true, std::memory_order_seq_cst);

      if (queue_.read(msgPtr, msgSize)) {
        sleeping_.store(false, std::memory_order_release);
        consumer_.post(message);
        waiter.reset();
        continue;
      }

      utils::futexWait(ftx_, ftxVal);
      sleeping_.store(false, std::memory_order_release);
      waiter.reset();
    }
    LOG_DEBUG("LfqLoop stop");
  }

private:
  const Optional<CoreId> coreId_;

  alignas(64) Queue queue_;
  alignas(64) AtomicBool running_{false};
  alignas(64) AtomicBool sleeping_{false};
  alignas(64) AtomicUInt32 ftx_{0};
  alignas(64) uint64_t wakeCounter_{0};

  alignas(64) ConsumerT &consumer_;
  BusT &bus_;
  std::jthread thread_;
};

} // namespace hft

#endif // HFT_COMMON_LFQRUNNER_HPP
