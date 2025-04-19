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

/**
 * @brief Tracks rtt values in the specified ranges
 * calculates the number of hits and the average values
 * @details HdrHistogram at home
 */
template <size_t... Ranges>
class RttTracker {
  static_assert(sizeof...(Ranges) > 0, "Need at least one range");
  static_assert(sizeof...(Ranges) < 2 || utils::is_ascending<Ranges...>(), "Ranges not ascending");

  static constexpr std::array<size_t, sizeof...(Ranges)> rangeValues = {Ranges...};
  static constexpr size_t RangeCount = sizeof...(Ranges) + 1;
  static constexpr size_t FlushTimeoutMs = 1000;

  /**
   * @brief Rtt values are logged from a several threads and flushed periodically
   */
  struct alignas(64) AtomicRttSample {
    std::atomic_uint64_t sum{0};
    Padding<std::atomic_uint64_t> p;
    std::atomic_uint64_t size{0};
  };
  struct AtomicRttStats {
    std::array<AtomicRttSample, RangeCount> samples{};
  };

public:
  struct RttSample {
    uint64_t sum{0};
    uint64_t size{0};
  };
  struct RttStats {
    std::array<RttSample, RangeCount> samples{};
  };

  static Timestamp logRtt(Timestamp timestamp) {
    thread_local AtomicRttStats stats;
    thread_local Timestamp lastFlushed = utils::getTimestamp();
    Timestamp current = utils::getTimestamp();
    auto rtt = (current - timestamp) / 1000;

    uint8_t scale = getRange(rtt);
    stats.samples[scale].sum += rtt;
    stats.samples[scale].size++;
    if (current - lastFlushed > FlushTimeoutMs) {
      for (int i = 0; i < RangeCount; ++i) {
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
    for (int i = 0; i < RangeCount; ++i) {
      stats.samples[i].sum = sGlobalStats.samples[i].sum.load(std::memory_order_relaxed);
      stats.samples[i].size = sGlobalStats.samples[i].size.load(std::memory_order_relaxed);
    }
    return stats;
  }

  static std::string getRttStats() {
    auto rttStats = getStats();
    auto &rtt = rttStats.samples;
    auto sizeTotal = 0;
    for (int i = 0; i < RangeCount; ++i) {
      sizeTotal += rtt[i].size;
    }
    if (sizeTotal == 0) {
      return "";
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);
    ss << "[";
    for (int i = 0; i < RangeCount; ++i) {
      if (i < RangeCount - 1) {
        ss << "<" << toScale(rangeValues[i]) << "|";
      } else {
        ss << ">" << toScale(rangeValues[i - 1]) << "]";
      }
    }
    ss << " ";
    for (int i = 0; i < RangeCount; ++i) {
      auto avg = ((rtt[i].size != 0) ? (rtt[i].sum / rtt[i].size) : 0);
      if (avg == 0) {
        ss << "0%";
      } else {
        ss << ((float)rtt[i].size / sizeTotal) * 100 << "% avg:";
        ss << toScale(avg);
      }
      if (i < RangeCount - 1) {
        ss << " ";
      }
    }
    return ss.str();
  }

private:
  static constexpr inline uint8_t getRange(Timestamp value) {
    for (int i = 0; i < RangeCount - 1; ++i) {
      if (value < rangeValues[i]) {
        return i;
      }
    }
    return RangeCount - 1;
  }
  static inline std::string toScale(Timestamp value) {
    bool us = value < 1000;
    if (!us) {
      value /= 1000;
    }
    return std::to_string(value) + (us ? "us" : "ms");
  }

  static AtomicRttStats sGlobalStats;
};

template <size_t... Ranges>
RttTracker<Ranges...>::AtomicRttStats RttTracker<Ranges...>::sGlobalStats;

} // namespace hft

#endif // HFT_COMMON_RTTTRACKER_HPP