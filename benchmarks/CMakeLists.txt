find_package(benchmark REQUIRED)

file(GLOB_RECURSE BENCH_SOURCES "src/*.cpp")

add_executable(hft_benchmark ${BENCH_SOURCES})
target_link_libraries(hft_benchmark PRIVATE hft_common benchmark::benchmark)
