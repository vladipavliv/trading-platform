/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_TESTS_TESTUTILS_HPP
#define HFT_TESTS_TESTUTILS_HPP

#include "config/config.hpp"
#include "container_types.hpp"
#include "primitive_types.hpp"

namespace hft::tests {

inline CoreId getCore(const Config &cfg, CoreId idx) {
  static const auto cores = cfg.get_vector<CoreId>("bench.bench_cores");
  return cores.at(idx);
}

} // namespace hft::tests

#endif // HFT_TESTS_TESTUTILS_HPP