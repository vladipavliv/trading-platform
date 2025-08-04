cmake_minimum_required(VERSION 3.10)
project(hft_platform)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.14.0  # Latest stable
)
set(INSTALL_GTEST OFF)
set(BUILD_GMOCK ON)

FetchContent_MakeAvailable(googletest)

set(CMAKE_CXX_COMPILER g++)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto=8 -DNDEBUG -fomit-frame-pointer") # -fno-rtti -fno-exceptions
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O1 -g3 -march=native -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer")

find_package(Boost 1.83 REQUIRED COMPONENTS thread)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
find_library(RDKAFKAXX_LIB rdkafka++)
find_package(PostgreSQL REQUIRED)
find_package(libpqxx REQUIRED)
find_package(benchmark REQUIRED)

pkg_check_modules(LIBPQXX REQUIRED libpqxx)

option(BUILD_TESTS "Build tests" OFF)
option(BUILD_BENCHMARKS "Build benchmarks" OFF)

message(STATUS "BUILD_TESTS is set to: ${BUILD_TESTS}")
message(STATUS "BUILD_BENCHMARKS is set to: ${BUILD_BENCHMARKS}")

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(client)
add_subdirectory(monitor)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(BUILD_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
