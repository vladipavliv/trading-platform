cmake_minimum_required(VERSION 3.10)
project(hft_platform)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -march=native -flto -fno-exceptions -fno-rtti -DNDEBUG")

add_definitions(-DBOOST_NO_EXCEPTIONS)

find_package(Boost 1.83 REQUIRED COMPONENTS thread fiber)
find_package(spdlog REQUIRED)

# Definitions
set(SERVER_URL "127.0.0.1")
set(PORT_TCP_IN 8080)
set(PORT_TCP_OUT 8081)
set(PORT_UDP 8082)
set(SERVER_CORE_COUNT 3)
set(TRADER_CORE_COUNT 3)
set(CORES_SERVER "2,4,6")
set(CORES_TRADER "3,5,7")
set(CACHE_LINE_SIZE 64)
set(USE_MIMALLOC ON)
set(FEED_RATE 10) # price update every 10ms
set(ORDER_RATE 10) # place new order every 100ms

# Generate flatbuffers code from the schema
set(SCHEMA_FILE ${CMAKE_SOURCE_DIR}/common/schema/marketdata.fbs)
set(GEN_DIR ${CMAKE_SOURCE_DIR}/common/src/gen)
add_custom_command(
    OUTPUT ${GEN_DIR}/marketdata_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${GEN_DIR} ${SCHEMA_FILE}
    DEPENDS ${SCHEMA_FILE}
    COMMENT "Generating FlatBuffers code from schema"
)
add_custom_target(code_generator DEPENDS ${GEN_DIR}/marketdata_generated.h)

set(CMAKE_AR "/usr/bin/ar")
set(CMAKE_RANLIB "/usr/bin/ranlib")

# PostgreSQL
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)

# make static library 
file(GLOB_RECURSE COMMON_SOURCES "common/src/*.cpp" "common/src/**/*.cpp" "common/src/*.hpp" "common/src/**/*.hpp")
add_library(hft_common STATIC ${COMMON_SOURCES})

target_include_directories(hft_common PUBLIC common/src common/src/gen common/src/types ${LIBPQXX_INCLUDE_DIRS})
target_link_libraries(hft_common PRIVATE ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES})

add_custom_command(TARGET hft_common POST_BUILD
    COMMAND ranlib ${CMAKE_CURRENT_BINARY_DIR}/libhft_common.a
    COMMENT "Running ranlib on libhft_common.a"
)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# Server Target
file(GLOB_RECURSE SERVER_SOURCES "server/src/*.cpp" "server/src/**/*.cpp" "server/src/*.hpp" "server/src/**/*.hpp")
add_executable(hft_server ${SERVER_SOURCES})
target_compile_definitions(hft_server PRIVATE 
    SERVER_URL="${SERVER_URL}" 
    PORT_TCP_IN=${PORT_TCP_IN} 
    PORT_TCP_OUT=${PORT_TCP_OUT} 
    PORT_UDP=${PORT_UDP} 
    CACHE_LINE_SIZE=${CACHE_LINE_SIZE}
    FEED_RATE=${FEED_RATE}
    CORES=${SERVER_CORE_COUNT}
    CORE_IDS="${CORES_SERVER}")
target_link_libraries(hft_server PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES})
target_include_directories(hft_server PRIVATE  common/src server/src server/src/types)

# Trader Target
file(GLOB_RECURSE TRADER_SOURCES "trader/src/*.cpp" "trader/src/**/*.cpp" "trader/src/*.hpp" "trader/src/**/*.hpp")
add_executable(hft_trader ${TRADER_SOURCES})
target_compile_definitions(hft_trader PRIVATE 
    SERVER_URL="${SERVER_URL}" 
    PORT_TCP_IN=${PORT_TCP_IN} 
    PORT_TCP_OUT=${PORT_TCP_OUT} 
    PORT_UDP=${PORT_UDP} 
    CACHE_LINE_SIZE=${CACHE_LINE_SIZE}
    ORDER_RATE=${ORDER_RATE}
    CORES=${TRADER_CORE_COUNT}
    CORE_IDS="${CORES_TRADER}")
target_link_libraries(hft_trader PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES})
target_include_directories(hft_trader PRIVATE common/src trader/src trader/src/types)

# mimalloc
if(USE_MIMALLOC)
    find_package(mimalloc REQUIRED)
    target_link_libraries(hft_server PRIVATE mimalloc)
    target_link_libraries(hft_trader PRIVATE mimalloc)
endif()

add_dependencies(hft_server code_generator)
add_dependencies(hft_trader code_generator)

add_custom_command(
    TARGET hft_trader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/common/config/config.ini ${CMAKE_BINARY_DIR}
)
