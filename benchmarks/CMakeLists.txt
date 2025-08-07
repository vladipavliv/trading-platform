file(GLOB_RECURSE BENCH_SOURCES 
    "src/*.cpp"
    "${PROJECT_SOURCE_DIR}/client/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/server/src/*.cpp"
)

list(FILTER BENCH_SOURCES EXCLUDE REGEX ".*/main.cpp$")

add_executable(benchmarks ${BENCH_SOURCES})

target_link_libraries(benchmarks PRIVATE 
    benchmark::benchmark 
    pthread 
    hft_common
)

target_include_directories(benchmarks PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/common/src
    ${CMAKE_SOURCE_DIR}/server/src
    ${CMAKE_SOURCE_DIR}/client/src
)

add_custom_command(
    TARGET benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/bench_server_config.ini ${CMAKE_BINARY_DIR}/benchmarks
)

add_custom_command(
    TARGET benchmarks POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/bench_client_config.ini ${CMAKE_BINARY_DIR}/benchmarks
)

target_compile_definitions(benchmarks PRIVATE BENCHMARK_BUILD)
