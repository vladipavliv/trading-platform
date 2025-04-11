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
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer")

find_package(Boost 1.83 REQUIRED COMPONENTS thread)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
find_library(RDKAFKAXX_LIB rdkafka++)

pkg_check_modules(LIBPQXX REQUIRED libpqxx)

enable_testing()

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(trader)
add_subdirectory(monitor)
add_subdirectory(test)
