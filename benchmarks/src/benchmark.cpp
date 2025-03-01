#include <benchmark/benchmark.h>

void someFunction() {}

static void BM_ServerFunction(benchmark::State &state) {
  for (auto _ : state) {
    someFunction();
  }
}

BENCHMARK(BM_ServerFunction);

BENCHMARK_MAIN();
