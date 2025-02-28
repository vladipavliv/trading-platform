cmake_minimum_required(VERSION 3.10)
project(hft_platform)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -DNDEBUG -fomit-frame-pointer") # -fno-rtti -fno-exceptions
set(CMAKE_CXX_FLAGS_DEBUG "-g -fsanitize=address -fno-omit-frame-pointer")

# add_definitions(-DBOOST_NO_EXCEPTIONS)

find_package(Boost 1.83 REQUIRED COMPONENTS thread fiber)
find_package(spdlog REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

set(USE_MIMALLOC OFF)

add_subdirectory(common)
add_subdirectory(server)
add_subdirectory(trader)
