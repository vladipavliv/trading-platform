/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_CONDITIONVARIABLE_HPP
#define HFT_COMMON_CONDITIONVARIABLE_HPP

#include <atomic>
#include <spdlog/spdlog.h>
#include <sys/eventfd.h>
#include <unistd.h>

namespace hft {

/**
 * @brief Lightweight condition variable to wake up threads
 * Okay after testing maybe its not as lightweight as one would have hoped, need some additional
 * logic like yeild thread for 100 cycles and then sleep on this fd or something
 */
class EventFd {
public:
  EventFd() : mEfd{eventfd(0, EFD_SEMAPHORE)} {
    if (mEfd == -1) {
      throw std::runtime_error("Failed to create eventfd");
    }
  }
  ~EventFd() {
    notify();
    close(mEfd);
  }

  void wait() {
    return; // Needs further improvement
    mWaiting.store(true, std::memory_order_relaxed);
    uint64_t val;
    if (read(mEfd, &val, sizeof(val)) == -1) {
      spdlog::error("Failed to read from eventfd");
    }
  }

  void notify() {
    return; // Needs further improvement
    if (!mWaiting.load(std::memory_order_relaxed)) {
      return;
    }
    uint64_t val{1};
    if (write(mEfd, &val, sizeof(val)) == -1) {
      spdlog::error("Failed to write to eventfd");
    }
  }

private:
  const int mEfd;
  std::atomic_bool mWaiting;
};

} // namespace hft

#endif // HFT_COMMON_CONDITIONVARIABLE_HPP