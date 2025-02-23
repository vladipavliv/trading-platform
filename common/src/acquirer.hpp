/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_ACQUIRER_HPP
#define HFT_COMMON_ACQUIRER_HPP

#include "template_types.hpp"

namespace hft {

template <typename Acquiree>
struct AcquiRer {
  AcquiRer(Acquiree &acquiree) : success{acquiree.acquire()} {
    if (success) {
      releaseHandler = [&acquiree]() { acquiree.release(); };
    }
  }
  ~AcquiRer() {
    if (releaseHandler) {
      releaseHandler();
    }
  }
  void release() {
    if (releaseHandler) {
      releaseHandler();
      releaseHandler = nullptr;
    }
  }
  Callback releaseHandler;
  const bool success;
};

} // namespace hft

#endif // HFT_COMMON_ACQUIRER_HPP