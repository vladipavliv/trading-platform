/**
 * @author Vladimir Pavliv
 * @date 2025-08-08
 */

#include <benchmark/benchmark.h>

#include "domain_types.hpp"
#include "inline_callable.hpp"
#include "types.hpp"
#include "utils/utils.hpp"

namespace hft::benchmarks {

static void BM_Op_StdFunctionCallableSizet(benchmark::State &state) {
  size_t value{0};

  std::function<void(const size_t &)> callable{[&value](CRef<size_t> t) { value += t; }};
  for (auto _ : state) {
    callable(value);
  }
  benchmark::DoNotOptimize(value);
}
BENCHMARK(BM_Op_StdFunctionCallableSizet);

static void BM_Op_StdFunctionCallableOrder(benchmark::State &state) {
  Order order = utils::generateOrder();
  size_t value{0};

  std::function<void(const Order &)> callable{[&value](CRef<Order> o) { value += o.id; }};
  for (auto _ : state) {
    callable(order);
  }
  benchmark::DoNotOptimize(value);
}
BENCHMARK(BM_Op_StdFunctionCallableOrder);

static void BM_Op_InlineCallableSizet(benchmark::State &state) {
  size_t value{0};

  InlineCallable<size_t> callable{[&value](CRef<size_t> t) { value += t; }};
  for (auto _ : state) {
    callable(value);
  }
  benchmark::DoNotOptimize(value);
}
BENCHMARK(BM_Op_InlineCallableSizet);

static void BM_Op_InlineCallableOrder(benchmark::State &state) {
  Order order = utils::generateOrder();
  size_t value{0};

  InlineCallable<Order> callable{[&value](CRef<Order> o) { value += o.id; }};
  for (auto _ : state) {
    callable(order);
  }
  benchmark::DoNotOptimize(value);
}
BENCHMARK(BM_Op_InlineCallableOrder);

} // namespace hft::benchmarks
