/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_LFQPOOL_HPP
#define HFT_COMMON_LFQPOOL_HPP

#include <atomic>
#include <map>
#include <memory>
#include <spdlog/spdlog.h>

#include "comparators.hpp"
#include "constants.hpp"
#include "locker.hpp"
#include "template_types.hpp"
#include "types.hpp"

namespace hft {

/**
 * @brief Pool to reuse lock free queues
 */
template <typename ValueType>
class LFQPool {
public:
  using Type = LFQPool<ValueType>;
  using LFQueueType = LFQueue<ValueType>;
  using SPtrLFQueueType = SPtrLFQueue<ValueType>;

  LFQPool(size_t size) {
    mPool.reserve(size);
    for (int i = 0; i < size; ++i) {
      mPool.emplace_back(std::make_shared<LFQueueType>(LFQ_SIZE));
    }
  }

  SPtrLFQueueType getLFQueue() {
    Lock<Type> lock{*this};
    if (!lock.success) {
      auto newLfq = std::make_shared<LFQueueType>(LFQ_SIZE);
      return newLfq;
    }
    auto lfqIter = std::find_if(mPool.begin(), mPool.end(),
                                [](const SPtrLFQueueType &lfq) { return lfq.use_count() == 1; });
    if (lfqIter != mPool.end()) {
      return *lfqIter;
    }
    auto newLfq = std::make_shared<LFQueueType>(LFQ_SIZE);
    mPool.emplace_back(newLfq);
    return newLfq;
  }

  inline bool lock() { return !mBusy.test_and_set(std::memory_order_acq_rel); }
  inline void unlock() { mBusy.clear(std::memory_order_acq_rel); }

private:
  std::atomic_flag mBusy = ATOMIC_FLAG_INIT;
  std::vector<SPtrLFQueueType> mPool;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
