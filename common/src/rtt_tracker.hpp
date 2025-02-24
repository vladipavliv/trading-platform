/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RTTTRACKER_HPP
#define HFT_COMMON_RTTTRACKER_HPP

#include <atomic>
#include <memory>
#include <set>
#include <vector>

#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

constexpr uint8_t SAMPLES_SIZE = 3;
constexpr uint32_t FLUSH_TIMEOUT_NS = 1000000;

struct RttRangeSample {
  uint64_t sum;
  uint64_t size;
};
struct RttStats {
  std::vector<RttRangeSample> samples{SAMPLES_SIZE};
};

/**
 * @brief HdrHistogram at home
 */
class RttTracker {
  struct alignas(64) GlobalSample {
    std::atomic_uint64_t sum;
    Padding<size_t> p;
    std::atomic_uint64_t size;
  };
  struct GlobalStats {
    std::vector<GlobalSample> samples{SAMPLES_SIZE};
  };

public:
  static TimestampRaw logRtt(TimestampRaw timestamp) {
    thread_local RttStats stats;
    thread_local TimestampRaw lastFlushed = utils::getLinuxTimestamp();
    TimestampRaw current = utils::getLinuxTimestamp();
    auto rtt = (current - timestamp) / 1000;

    uint8_t scale = getScale(rtt);
    stats.samples[scale].sum += rtt;
    stats.samples[scale].size++;
    if (current - lastFlushed > FLUSH_TIMEOUT_NS) {
      for (int i = 0; i < SAMPLES_SIZE; ++i) {
        sGlobalStats.samples[i].sum.fetch_add(stats.samples[i].sum, std::memory_order_relaxed);
        sGlobalStats.samples[i].size.fetch_add(stats.samples[i].size, std::memory_order_relaxed);
        stats.samples[i].sum = 0;
        stats.samples[i].size = 0;
      }
      lastFlushed = current;
    }
    return rtt;
  }

  static RttStats getStats() {
    RttStats stats;
    for (int i = 0; i < SAMPLES_SIZE; ++i) {
      stats.samples[i].sum = sGlobalStats.samples[i].sum.load(std::memory_order_relaxed);
      stats.samples[i].size = sGlobalStats.samples[i].size.load(std::memory_order_relaxed);
    }
    return stats;
  }

private:
  static inline uint8_t getScale(TimestampRaw value) {
    return (value < 100) ? 0 : ((value < 1000) ? 1 : 2);
  }

  static GlobalStats sGlobalStats;
};

RttTracker::GlobalStats RttTracker::sGlobalStats;

} // namespace hft

#endif // HFT_COMMON_RTTTRACKER_HPP