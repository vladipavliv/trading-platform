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

#include "acquirer.hpp"
#include "comparators.hpp"
#include "constants.hpp"
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
    AcquiRer<Type> ack{*this};
    if (!ack.success) {
      auto newLfq = std::make_shared<LFQueueType>(LFQ_SIZE);
      // Wanted to use lock free buffer here to add newly created lfq to a pool later but shared_ptr
      // aint trivially destructible. Not a big deal i think as this right here is quite a tight
      // window to squeeze in, and its just one lfq that wont be reused
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

  inline bool acquire() { return !mBusy.test_and_set(std::memory_order_acq_rel); }
  inline void release() { mBusy.clear(std::memory_order_acq_rel); }

private:
  std::atomic_flag mBusy = ATOMIC_FLAG_INIT;
  std::vector<SPtrLFQueueType> mPool;
};

} // namespace hft

#endif // HFT_COMMON_BUFFERPOOL_HPP
