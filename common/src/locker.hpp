/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_LOCKER_HPP
#define HFT_COMMON_LOCKER_HPP

#include <thread>

#include "template_types.hpp"

namespace hft {

template <typename Lockee>
struct Lock {
  Lock(Lockee &obj) {
    size_t counter{0};
    while (++counter < 100) {
      if (obj.lock()) {
        success = true;
        unlocker = [&obj]() { obj.unlock(); };
        break;
      }
      std::this_thread::yield();
    }
  }
  ~Lock() { unlock(); }

  void unlock() {
    if (unlocker) {
      unlocker();
      unlocker = nullptr;
    }
  }
  Callback unlocker{nullptr};
  bool success{false};
};

} // namespace hft

#endif // HFT_COMMON_LOCKER_HPP