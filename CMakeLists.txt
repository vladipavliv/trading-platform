cmake_minimum_required(VERSION 3.10)

if(NOT CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "/usr/bin/g++" CACHE STRING "C++ compiler" FORCE)
endif()

project(hft_platform)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto=8 -DNDEBUG -fomit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -g3 -march=native -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fsanitize=undefined -O1 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG_VALGRIND "-g -O0 -fno-omit-frame-pointer")
set(CMAKE_LINKER_FLAGS_DEBUG "-fsanitize=address,undefined")

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(TELEMETRY_ENABLED "Enable telemetry" OFF)
set(SPDLOG_ACTIVE_LEVEL "SPDLOG_LEVEL_ERROR" CACHE STRING "spdlog active level")

if(DEFINED ENV{GITHUB_ACTIONS})
  message(STATUS "CI/CD build detected: GitHub Actions")
  set(IS_CICD_BUILD ON)
else()
  message(STATUS "Local build")
  set(IS_CICD_BUILD OFF)
endif()

if(IS_CICD_BUILD)
  set(SERIALIZATION "FBS")
  set(BUILD_TESTS ON CACHE BOOL "Build tests" FORCE)
  set(BUILD_BENCHMARKS OFF CACHE BOOL "Build benchmarks" FORCE)
else()
  if(TELEMETRY_ENABLED)
    add_compile_definitions(TELEMETRY_ENABLED)
  endif()
  if(BUILD_TESTS)
    set(SERIALIZATION "FBS")
  else()
    set(SERIALIZATION "SBE")
  endif()
endif()

if(SERIALIZATION STREQUAL "FBS")
  add_compile_definitions(SERIALIZATION_FBS)
elseif(SERIALIZATION STREQUAL "SBE")
  add_compile_definitions(SERIALIZATION_SBE)
endif()

find_package(Boost 1.83 REQUIRED COMPONENTS thread)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
find_library(RDKAFKAXX_LIB rdkafka++)
find_package(PostgreSQL REQUIRED)
find_package(libpqxx REQUIRED)
find_package(benchmark REQUIRED)
find_package(folly REQUIRED)
find_package(Glog REQUIRED)
find_package(Protobuf REQUIRED)

pkg_check_modules(LIBPQXX REQUIRED libpqxx)

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(monitor)

include(FetchContent)

if(BUILD_TESTS)
  enable_testing()

  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
  )
  set(INSTALL_GTEST OFF)
  set(BUILD_GMOCK ON)

  FetchContent_MakeAvailable(googletest)

  add_subdirectory(tests)
endif()

string(ASCII 27 esc)

set(vars
    CMAKE_BUILD_TYPE
    BUILD_TESTS
    BUILD_BENCHMARKS
    TELEMETRY_ENABLED
    SPDLOG_ACTIVE_LEVEL
    SERIALIZATION
)

# Find maximum variable name length
set(max_len 0)
foreach(v ${vars})
    string(LENGTH "${v}" len)
    if(len GREATER max_len)
        set(max_len ${len})
    endif()
endforeach()

# Print with padding + ANSI colors
foreach(v ${vars})
    string(LENGTH "${v}" len)
    math(EXPR pad "${max_len} - ${len}")
    string(REPEAT " " ${pad} spaces)

    # Use NOTICE instead of STATUS so ANSI codes are preserved
    message(NOTICE
        "${esc}[1;32m${v}${spaces}${esc}[0m : ${esc}[1;33m${${v}}${esc}[0m"
    )
endforeach()
