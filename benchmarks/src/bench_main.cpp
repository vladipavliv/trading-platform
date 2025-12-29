/**
 * @author Vladimir Pavliv
 * @date 2025-04-07
 */

#include <benchmark/benchmark.h>

#include "bench_server.hpp"

int main(int argc, char **argv) {
  ::benchmark::Initialize(&argc, argv);

  if (::benchmark::ReportUnrecognizedArguments(argc, argv))
    return 1;
  ::benchmark::RunSpecifiedBenchmarks();

  return 0;
}
