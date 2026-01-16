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
  static constexpr std::array<size_t, sizeof...(Ranges)> rangeValues{Ranges...};
  static constexpr size_t RangeCount = sizeof...(Ranges) + 1;

  struct RttSample {
    uint64_t sum{0};
    uint64_t size{0};
  };

  struct alignas(64) RttStats {
    std::array<RttSample, RangeCount> samples{};
    alignas(64) uint64_t globalMax{0};
  };

public:
  struct Snapshot {
    struct Sample {
      uint64_t sum{0};
      uint64_t size{0};
    };
    std::array<Sample, RangeCount> samples;
    uint64_t globalMax;

    uint64_t volume() const {
      uint64_t total = 0;
      for (const auto &s : samples) {
        total += s.size;
      }
      return total;
    }
  };

  static uint64_t logRtt(Timestamp rttNs) {
    const uint8_t bucket = getRange(rttNs);

    auto &s = sGlobalStats.samples[bucket];
    s.sum += rttNs;
    s.size += 1;

    if (rttNs > sGlobalStats.globalMax) [[unlikely]] {
      sGlobalStats.globalMax = rttNs;
    }

    return rttNs;
  }

  static Snapshot getStats() {
    std::atomic_thread_fence(std::memory_order_acquire);

    Snapshot snap;
    for (size_t i = 0; i < RangeCount; ++i) {
      snap.samples[i].sum = sGlobalStats.samples[i].sum;
      snap.samples[i].size = sGlobalStats.samples[i].size;
    }
    snap.globalMax = sGlobalStats.globalMax;
    return snap;
  }

  static String toString(CRef<Snapshot> stats) {
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
        ss << pct << "%";
        // ss << "(" << toShortCount(s.size) << ")"
        ss << " avg:" << toScaleNs(avgNs);
      }
      ss << " | ";
    }

    ss << "Max:" << toScale(stats.globalMax);
    return ss.str();
  }

  static void reset() {
    for (size_t i = 0; i < RangeCount; ++i) {
      sGlobalStats.samples[i].sum = 0;
      sGlobalStats.samples[i].size = 0;
    }

    sGlobalStats.globalMax = 0;
    std::atomic_thread_fence(std::memory_order_release);
  }

private:
  static constexpr uint8_t getRange(uint64_t ns) {
    uint8_t bucket = 0;
    ((bucket += (ns >= Ranges)), ...);
    return bucket;
  }

  static String toScale(uint64_t ns) {
    if (ns < 1'000)
      return std::to_string(ns) + "ns";
    if (ns < 1'000'000)
      return std::to_string(ns / 1'000) + "µs";
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
