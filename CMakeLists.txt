cmake_minimum_required(VERSION 3.10)
project(hft_platform)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(BUILD_TESTS)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0  # Latest stable
  )
  set(INSTALL_GTEST OFF)
  set(BUILD_GMOCK ON)

  FetchContent_MakeAvailable(googletest)
endif()

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto=8 -DNDEBUG -fomit-frame-pointer") # -fno-rtti -fno-exceptions
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -g3 -march=native -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address,undefined -fno-omit-frame-pointer")                              
set(CMAKE_LINKER_FLAGS_DEBUG "-fsanitize=address,undefined")

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

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)
option(TELEMETRY_ENABLED "Enable telemetry" OFF)

string(ASCII 27 esc)
message(STATUS "${esc}[1;32mBUILD_TESTS${esc}[0m is set to: ${esc}[1;33m${BUILD_TESTS}${esc}[0m")
message(STATUS "${esc}[1;32mBUILD_BENCHMARKS${esc}[0m is set to: ${esc}[1;33m${BUILD_BENCHMARKS}${esc}[0m")
message(STATUS "${esc}[1;32mTELEMETRY_ENABLED${esc}[0m is set to: ${esc}[1;33m${TELEMETRY_ENABLED}${esc}[0m")
message(STATUS "${esc}[1;32mSPDLOG_ACTIVE_LEVEL${esc}[0m is set to: ${esc}[1;33m${SPDLOG_ACTIVE_LEVEL}${esc}[0m")

if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(TELEMETRY_ENABLED)
    add_compile_definitions(TELEMETRY_ENABLED)
endif()

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(monitor)
