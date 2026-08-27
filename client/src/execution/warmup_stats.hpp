/**
 * @author Vladimir Pavliv
 * @date 2026-08-25
 */

#ifndef HFT_SERVER_WARMUPSTATS_HPP
#define HFT_SERVER_WARMUPSTATS_HPP

#include "config/client_config.hpp"
#include "logging.hpp"
#include "primitive_types.hpp"

namespace hft::client {

struct WarmupStats {
  enum class State : uint8_t { NotStarted, InProgress, Finished };

  const ClientConfig &config;
  const size_t countTotal;
  size_t count{0};
  State state{State::NotStarted};

  std::vector<Timestamp> startTimestamps;
  std::vector<Timestamp> endTimestamps;

  explicit WarmupStats(const ClientConfig &cfg)
      : config{cfg}, countTotal{config.data.get<size_t>("rates.warmup")} {
    startTimestamps.resize(countTotal);
    endTimestamps.resize(countTotal);
  }

  void finalize() {
    const size_t numSections = 10;
    size_t sectionSize = (count + numSections - 1) / numSections;

    LOG_INFO_SYSTEM("Warmup results: {} samples, {} sections", count, numSections);

    for (size_t section = 0; section < numSections; ++section) {
      size_t startIdx = section * sectionSize;
      size_t endIdx = std::min(startIdx + sectionSize, count);

      if (startIdx >= count) {
        break;
      }

      size_t sectionCount = endIdx - startIdx;

      size_t rttTotal = 0;
      size_t rttMin = SIZE_MAX;
      size_t rttMax = 0;

      for (size_t i = startIdx; i < endIdx; ++i) {
        const size_t rtt = endTimestamps[i] - startTimestamps[i];
        rttTotal += rtt;
        rttMin = std::min(rttMin, rtt);
        rttMax = std::max(rttMax, rtt);
      }

      rttTotal = (rttTotal / sectionCount) * config.nsPerCycle;
      rttMin *= config.nsPerCycle;
      rttMax *= config.nsPerCycle;

      LOG_INFO_SYSTEM("Section {}/{}: count={} Avg={}ns Min={}ns Max={}ns", section + 1,
                      numSections, sectionCount, rttTotal, rttMin, rttMax);
    }

    startTimestamps.clear();
    endTimestamps.clear();
    state = WarmupStats::State::Finished;
  }
};
} // namespace hft::client

#endif // HFT_SERVER_WARMUPSTATS_HPP