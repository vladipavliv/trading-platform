/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <benchmark/benchmark.h>

#include "domain_types.hpp"
#include "primitive_types.hpp"
#include "utils/data_generator.hpp"
#include "utils/handler.hpp"
#include "utils/rng.hpp"
#include "utils/test_utils.hpp"

namespace hft::benchmarks {

using namespace utils;
using namespace tests;

struct Processor {
  Order order;
  void onEvent(const Order &o) {
    order = o;
    benchmark::DoNotOptimize(order);
    benchmark::ClobberMemory();
  }
};

static void DISABLED_BM_UtilsRawMemberCall(benchmark::State &state) {
  Processor proc;
  Order o = genOrder();
  for (auto _ : state) {
    benchmark::DoNotOptimize(o);
    o.id++;
    proc.onEvent(o);
    benchmark::DoNotOptimize(proc.order);
  }
}
BENCHMARK(DISABLED_BM_UtilsRawMemberCall);

static void DISABLED_BM_UtilsCustomHandler(benchmark::State &state) {
  Processor proc;
  Order o = genOrder();
  auto handler = Handler<void(const Order &)>::bind<Processor, &Processor::onEvent>(&proc);
  for (auto _ : state) {
    benchmark::DoNotOptimize(o);
    o.id++;
    handler(o);
    benchmark::DoNotOptimize(proc.order);
  }
}
BENCHMARK(DISABLED_BM_UtilsCustomHandler);

static void DISABLED_BM_UtilsStdFunction(benchmark::State &state) {
  Processor proc;
  Order o = genOrder();
  std::function<void(const Order &)> func = [&](const Order &ev) { proc.onEvent(ev); };
  for (auto _ : state) {
    benchmark::DoNotOptimize(o);
    o.id++;
    func(o);
    benchmark::DoNotOptimize(proc.order);
  }
}
BENCHMARK(DISABLED_BM_UtilsStdFunction);

static void DISABLED_BM_BenchRng(benchmark::State &state) {
  benchmark::DoNotOptimize(RNG::generate<size_t>(0, 1));
  size_t value{0};

  for (auto _ : state) {
    value = RNG::generate<size_t>(0, value);
    benchmark::DoNotOptimize(value);
  }
}
BENCHMARK(DISABLED_BM_BenchRng);

} // namespace hft::benchmarks
