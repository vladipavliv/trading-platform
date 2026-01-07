/**
 * @author Vladimir Pavliv
 * @date 2025-12-30
 */

#ifndef HFT_BENCH_BENCHUTILS_HPP
#define HFT_BENCH_BENCHUTILS_HPP

#include "config/config.hpp"
#include "container_types.hpp"
#include "primitive_types.hpp"

namespace hft::benchmarks {

inline CoreId getCore(CoreId idx) {
  static const auto cores = Config::get<Vector<CoreId>>("bench.bench_cores");
  return cores.at(idx);
}

} // namespace hft::benchmarks

#endif // HFT_BENCH_BENCHUTILS_HPP