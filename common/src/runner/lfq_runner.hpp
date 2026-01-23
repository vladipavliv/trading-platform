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
#include "utils/handler.hpp"
#include "utils/spin_wait.hpp"
#include "utils/sync_utils.hpp"
#include "utils/thread_utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageT, typename ConsumerT, typename BusT, size_t Capacity = 65536>
class LfqRunner {
  using SelfT = LfqRunner<MessageT, ConsumerT, BusT, Capacity>;
  using Queue = SequencedSPSC<Capacity>;

public:
  LfqRunner(ConsumerT &consumer, BusT &bus, std::stop_token stopToken, String name,
            Optional<CoreId> coreId = std::nullopt, bool feedFromBus = false)
      : consumer_{consumer}, bus_{bus}, stopToken_{std::move(stopToken)}, name_{std::move(name)},
        coreId_{coreId} {
    if (feedFromBus) {
      bus_.subscribe(CRefHandler<MessageT>::template bind<SelfT, &SelfT::post>(this));
    }
  }

  ~LfqRunner() { LOG_DEBUG_SYSTEM("~LfqRunner {}", name_); }

  void run(StdCallback onReadyClb = nullptr) {
    LOG_DEBUG("LfqRunner run {}", name_);
    thread_ = std::jthread([this, clb = std::move(onReadyClb)]() {
      try {
        started_.store(true, std::memory_order_release);

        utils::setThreadRealTime();
        if (coreId_.has_value()) {
          LOG_INFO("LfqRunner {} started on core {}", name_, coreId_.value());
          utils::pinThreadToCore(coreId_.value());
        } else {
          LOG_INFO("LfqRunner {} started", name_);
        }
        if (clb) {
          clb();
        }
        lfqLoop();
      } catch (const std::exception &ex) {
        const auto error = std::format("std exception in LfqRunner {}: {}", name_, ex.what());
        LOG_ERROR_SYSTEM("{}", error);
        bus_.post(InternalError{StatusCode::Error, error});
      } catch (...) {
        const auto error = std::format("unknown exception in LfqRunner {}", name_);
        LOG_ERROR_SYSTEM("{}", error);
        bus_.post(InternalError{StatusCode::Error, error});
      }
      LOG_DEBUG("LfqRunner finished {}", name_);
    });
  }

  void stop() {
    LOG_DEBUG("Stopping LfqRunner {}", name_);
    if (!started_.load(std::memory_order_acquire)) {
      return;
    }
    started_.store(false, std::memory_order_release);
    auto val = ftx_.fetch_add(1, std::memory_order_release);
    LOG_DEBUG("futex wake {} {}", name_, val);
    utils::futexWake(ftx_);
    utils::join(thread_);
    LOG_DEBUG("LfqRunner {} stopped", name_);
  }

  inline void post(CRef<MessageT> message) {
    LOG_DEBUG("{}", toString(message));
    if (stopToken_.stop_requested()) {
      return;
    }
    auto msgPtr = reinterpret_cast<const uint8_t *>(&message);
    auto msgSize = sizeof(MessageT);

    SpinWait waiter;
    while (!queue_.write(msgPtr, msgSize)) {
      if (!++waiter) {
        bus_.post(
            InternalError{StatusCode::Error, std::format("Failed to post to LfqRunner {}", name_)});
        break;
      }
    }
    if (sleeping_.load(std::memory_order_seq_cst)) [[unlikely]] {
      ftx_.fetch_add(1, std::memory_order_release);
      utils::futexWake(ftx_);
    }
  }

private:
  void lfqLoop() {
    LOG_DEBUG_SYSTEM("LfqRunner::lfqLoop {} enter", name_);
    MessageT message;
    auto msgPtr = reinterpret_cast<uint8_t *>(&message);
    auto msgSize = sizeof(MessageT);

    SpinWait waiter;
    while (!stopToken_.stop_requested()) {
      if (queue_.read(msgPtr, msgSize)) {
        waiter.reset();
        do {
          consumer_.post(message);
        } while (queue_.read(msgPtr, msgSize) && !stopToken_.stop_requested());
        continue;
      }
      if (++waiter || stopToken_.stop_requested()) {
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

      if (stopToken_.stop_requested()) {
        break;
      }

      LOG_DEBUG("futex sleep {} {}", name_, ftxVal);
      utils::futexWait(ftx_, ftxVal);
      LOG_DEBUG("futex awake {} {}", name_, ftxVal);
      sleeping_.store(false, std::memory_order_release);
      waiter.reset();
    }
    LOG_DEBUG_SYSTEM("LfqRunner::lfqLoop {} leave", name_);
  }

private:
  ConsumerT &consumer_;
  BusT &bus_;
  std::stop_token stopToken_;
  const String name_;
  const Optional<CoreId> coreId_;

  Queue queue_;
  AtomicBool started_{false};
  ALIGN_CL AtomicBool sleeping_{false};
  ALIGN_CL AtomicUInt32 ftx_{0};

  std::jthread thread_;
};

} // namespace hft

#endif // HFT_COMMON_LFQRUNNER_HPP
