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
#include "utils/thread_utils.hpp"

namespace hft {

/**
 * @brief
 */
template <typename MessageT, typename ConsumerT, typename BusT, size_t Capacity = 65536>
class LfqRunner {
  using Queue = SequencedSPSC<Capacity>;

public:
  LfqRunner(ConsumerT &consumer, BusT &bus, String name, Optional<CoreId> coreId = std::nullopt,
            bool feedFromBus = false)
      : consumer_{consumer}, bus_{bus}, name_{std::move(name)}, coreId_{coreId} {
    if (feedFromBus) {
      using SelfT = LfqRunner<MessageT, ConsumerT, BusT, Capacity>;
      bus_.template subscribe<MessageT>(
          CRefHandler<MessageT>::template bind<SelfT, &SelfT::post>(this));
    }
  }

  ~LfqRunner() { LOG_DEBUG_SYSTEM("~LfqRunner {}", name_); }

  void run() {
    LOG_DEBUG("LfqRunner run {}", name_);
    thread_ = std::jthread([this]() {
      try {
        running_.store(true);
        running_.notify_all();

        utils::setThreadRealTime();
        if (coreId_.has_value()) {
          LOG_INFO("LfqRunner {} started on core {}", name_, coreId_.value());
          utils::pinThreadToCore(coreId_.value());
        } else {
          LOG_INFO("LfqRunner {} started", name_);
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
    running_.wait(false);
  }

  void stop() {
    LOG_DEBUG("Stopping LfqRunner {}", name_);
    if (!running_.load(std::memory_order_acquire)) {
      LOG_ERROR("Already stopped");
      return;
    }
    running_.store(false, std::memory_order_release);
    auto val = ftx_.fetch_add(1, std::memory_order_release);
    LOG_DEBUG("futex wake {} {}", name_, val);
    utils::futexWake(ftx_);
    utils::join(thread_);
    LOG_DEBUG("LfqRunner {} stopped", name_);
  }

  inline void post(CRef<MessageT> message) {
    LOG_DEBUG("{}", toString(message));
    if (!running_.load(std::memory_order_acquire)) {
      LOG_ERROR("Already stopped");
      return;
    }
    auto msgPtr = reinterpret_cast<const uint8_t *>(&message);
    auto msgSize = sizeof(MessageT);

    SpinWait waiter;
    while (!queue_.write(msgPtr, msgSize)) {
      if (!++waiter) {
        const auto err = String("Failed to post to LfqRunner ") + name_;
        bus_.post(InternalError{StatusCode::Error, err});
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
    while (running_.load(std::memory_order_acquire)) {
      if (queue_.read(msgPtr, msgSize)) {
        waiter.reset();
        do {
          consumer_.post(message);
        } while (queue_.read(msgPtr, msgSize) && running_.load(std::memory_order_acquire));
        continue;
      }
      if (++waiter || !running_.load(std::memory_order_acquire)) {
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

      if (!running_.load(std::memory_order_acquire)) {
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
  ALIGN_CL ConsumerT &consumer_;
  ALIGN_CL BusT &bus_;
  ALIGN_CL const String name_;
  ALIGN_CL const Optional<CoreId> coreId_;

  ALIGN_CL Queue queue_;
  ALIGN_CL AtomicBool running_{false};
  ALIGN_CL AtomicBool sleeping_{false};
  ALIGN_CL AtomicUInt32 ftx_{0};

  ALIGN_CL std::jthread thread_;
};

} // namespace hft

#endif // HFT_COMMON_LFQRUNNER_HPP
