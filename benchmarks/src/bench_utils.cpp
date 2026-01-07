/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <benchmark/benchmark.h>

#include "primitive_types.hpp"
#include "utils/rng.hpp"

namespace hft::benchmarks {

using namespace utils;

static void DISABLED_BM_Op_BenchRng(benchmark::State &state) {
  benchmark::DoNotOptimize(RNG::generate<size_t>(0, 1));
  size_t value{0};

  for (auto _ : state) {
    value = RNG::generate<size_t>(0, value);
    benchmark::DoNotOptimize(value);
  }
}
BENCHMARK(DISABLED_BM_Op_BenchRng);

} // namespace hft::benchmarks
