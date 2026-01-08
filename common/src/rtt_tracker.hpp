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

#include "primitive_types.hpp"
#include "utils/time_utils.hpp"

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

  struct alignas(64) RttSample {
    AtomicUint64 sum{0};
    AtomicUint64 size{0};
  };

  struct RttStats {
    std::array<RttSample, RangeCount> samples{};
    alignas(64) AtomicUint64 globalMax{0};
  };

public:
  struct Snapshot {
    struct Sample {
      uint64_t sum;
      uint64_t size;
    };
    std::array<Sample, RangeCount> samples;
    uint64_t globalMax;
  };

  static uint64_t logRtt(Timestamp start, Timestamp end) {
    if (start > end) [[unlikely]]
      std::swap(start, end);

    const uint64_t rttNs = end - start;
    const uint8_t bucket = getRange(rttNs);

    auto &s = sGlobalStats.samples[bucket];
    s.sum.fetch_add(rttNs, std::memory_order_relaxed);
    s.size.fetch_add(1, std::memory_order_relaxed);

    if (rttNs > sGlobalStats.globalMax.load(std::memory_order_relaxed)) {
      sGlobalStats.globalMax.store(rttNs, std::memory_order_relaxed);
    }

    return rttNs;
  }

  static Snapshot getStats() {
    Snapshot snap;
    for (size_t i = 0; i < RangeCount; ++i) {
      auto &s = sGlobalStats.samples[i];
      snap.samples[i].sum = s.sum.load(std::memory_order_relaxed);
      snap.samples[i].size = s.size.load(std::memory_order_relaxed);
    }
    snap.globalMax = sGlobalStats.globalMax.load(std::memory_order_relaxed);
    return snap;
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
        ss << pct << "%(" << toShortCount(s.size) << ") avg:" << toScaleNs(avgNs);
      }
      if (i + 1 < RangeCount) {
        ss << " ";
      }
    }

    ss << " max:" << toScale(stats.globalMax);
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

  static String toScale(uint64_t ns) {
    if (ns < 1'000)
      return std::to_string(ns) + "ns";
    if (ns < 1'000'000)
      return std::to_string(ns / 1'000) + "us";
    if (ns < 1'000'000'000)
      return std::to_string(ns / 1'000'000) + "ms";
    return std::to_string(ns / 1'000'000'000) + "s";
  }

  static String toShortCount(uint64_t count) {
    if (count < 1'000) {
      return std::to_string(count);
    }
    std::stringstream ss;
    ss << std::fixed << std::setprecision(1);
    if (count < 1'000'000) {
      ss << (count / 1000.0) << "K";
      return ss.str();
    }
    ss << (count / 1000000.0) << "M";
    return ss.str();
  }

  static String toScaleNs(double ns) { return toScale(static_cast<uint64_t>(ns)); }

private:
  inline static RttStats sGlobalStats;
};

} // namespace hft

#endif // HFT_COMMON_RTTTRACKER_HPP
