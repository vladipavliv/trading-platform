/**
 * @author Vladimir Pavliv
 * @date 2025-02-13
 */

#ifndef HFT_COMMON_WARMUPJOB_HPP
#define HFT_COMMON_WARMUPJOB_HPP

#include <atomic>
#include <cmath>

#include "boost_types.hpp"
#include "config/config.hpp"
#include "logger.hpp"
#include "types.hpp"

namespace hft {

class WarmUpJob {
public:
  WarmUpJob(IoContext ctx) : timer_{ctx} {}

  void run() {
    timer_.expires_after(Config::cfg.warmUp);
    timer_.async_wait([this](BoostErrorRef ec) { stop_.store(true); });

    double dummyCounter = 0;
    while (!stop_) {
      for (int i = 0; i < 1000000; ++i) {
        dummyCounter += std::sin(i) * std::cos(i);
      }
    }
  }

private:
  std::atomic_bool stop_{false};
  SteadyTimer timer_;
};

} // namespace hft

#endif // HFT_COMMON_WARMUPJOB_HPP