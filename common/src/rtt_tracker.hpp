/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_RTTTRACKER_HPP
#define HFT_COMMON_RTTTRACKER_HPP

#include <array>
#include <atomic>
#include <iomanip>
#include <sstream>

#include "types.hpp"
#include "utils/utils.hpp"

namespace hft {

/**
 * @brief
 */
template <size_t... Ranges>
class RttTracker {
  static_assert(sizeof...(Ranges) > 0, "Need at least one range");
  static_assert(sizeof...(Ranges) < 2 || utils::is_ascending<Ranges...>(),
                "Ranges must be ascending");

  static constexpr std::array<size_t, sizeof...(Ranges)> rangeValues{Ranges...};
  static constexpr size_t RangeCount = sizeof...(Ranges) + 1;
  static constexpr uint64_t FlushTimeoutNs = 1'000'000'000ULL;

  struct alignas(64) AtomicRttSample {
    std::atomic<uint64_t> sum{0};
    std::atomic<uint64_t> size{0};
  };
  static_assert(sizeof(AtomicRttSample) == 64);

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

  static Timestamp logRtt(Timestamp first, bool multiThreaded = true) {
    return logRtt(first, utils::getTimestampNs(), multiThreaded);
  }

  static Timestamp logRtt(Timestamp first, Timestamp second, bool multiThreaded = true) {
    if (first > second) {
      std::swap(first, second);
    }

    const uint64_t rttNs = second - first;
    const uint8_t bucket = getRange(rttNs);

    if (multiThreaded) {
      thread_local RttStats local{};
      thread_local Timestamp lastFlush = utils::getTimestampNs();

      local.samples[bucket].sum += rttNs;
      local.samples[bucket].size++;

      if (second - lastFlush >= FlushTimeoutNs) {
        flushLocal(local);
        lastFlush = second;
      }
    } else {
      auto &s = sGlobalStats.samples[bucket];
      s.sum.fetch_add(rttNs, std::memory_order_relaxed);
      s.size.fetch_add(1, std::memory_order_relaxed);
    }

    return rttNs;
  }

  static RttStats getStats() {
    RttStats stats{};
    for (size_t i = 0; i < RangeCount; ++i) {
      stats.samples[i].sum = sGlobalStats.samples[i].sum.load(std::memory_order_relaxed);
      stats.samples[i].size = sGlobalStats.samples[i].size.load(std::memory_order_relaxed);
    }
    return stats;
  }

  static String getStatsString() {
    const auto stats = getStats();

    uint64_t totalCount = 0;
    for (const auto &s : stats.samples) {
      totalCount += s.size;
    }
    if (totalCount == 0) {
      return {};
    }

    std::stringstream ss;
    ss << std::fixed << std::setprecision(2);

    ss << "[";
    for (size_t i = 0; i < RangeCount; ++i) {
      if (i + 1 < RangeCount) {
        ss << "<" << toScale(rangeValues[i]) << "|";
      } else {
        ss << ">" << toScale(rangeValues[i - 1]) << "]";
      }
    }

    ss << " ";
    for (size_t i = 0; i < RangeCount; ++i) {
      const auto &s = stats.samples[i];
      if (s.size == 0) {
        ss << "0%";
      } else {
        const double pct = (double(s.size) / totalCount) * 100.0;
        const double avgNs = double(s.sum) / s.size;
        ss << pct << "% avg:" << toScaleNs(avgNs);
      }
      if (i + 1 < RangeCount) {
        ss << " ";
      }
    }

    return ss.str();
  }

private:
  static constexpr uint8_t getRange(uint64_t ns) {
    for (size_t i = 0; i < RangeCount - 1; ++i) {
      if (ns < rangeValues[i]) {
        return i;
      }
    }
    return RangeCount - 1;
  }

  static void flushLocal(RttStats &local) {
    for (size_t i = 0; i < RangeCount; ++i) {
      if (local.samples[i].size != 0) {
        sGlobalStats.samples[i].sum.fetch_add(local.samples[i].sum, std::memory_order_relaxed);
        sGlobalStats.samples[i].size.fetch_add(local.samples[i].size, std::memory_order_relaxed);
        local.samples[i] = {};
      }
    }
  }

  static String toScale(uint64_t ns) {
    if (ns < 1'000)
      return std::to_string(ns) + "ns";
    if (ns < 1'000'000)
      return std::to_string(ns / 1'000) + "us";
    if (ns < 1'000'000'000)
      return std::to_string(ns / 1'000'000) + "ms";
    return std::to_string(ns / 1'000'000'000) + "s";
  }

  static String toScaleNs(double ns) { return toScale(static_cast<uint64_t>(ns)); }

private:
  static AtomicRttStats sGlobalStats;
};

template <size_t... Ranges>
typename RttTracker<Ranges...>::AtomicRttStats RttTracker<Ranges...>::sGlobalStats{};

} // namespace hft

#endif // HFT_COMMON_RTTTRACKER_HPP
