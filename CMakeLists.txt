cmake_minimum_required(VERSION 3.10)
project(hft_platform)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

find_package(Boost 1.83 REQUIRED COMPONENTS thread fiber)
find_package(spdlog REQUIRED)

set(SCHEMA_FILE ${CMAKE_SOURCE_DIR}/common/schema/marketdata.fbs)
set(GEN_DIR ${CMAKE_SOURCE_DIR}/common/src/gen)

# Generate fb code
add_custom_command(
    OUTPUT ${GEN_DIR}/marketdata_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${GEN_DIR} ${SCHEMA_FILE}
    DEPENDS ${SCHEMA_FILE}
    COMMENT "Generating FlatBuffers code from schema"
)

add_custom_target(code_generator
    DEPENDS ${GEN_DIR}/marketdata_generated.h
)

# Collect common code sources
file(GLOB_RECURSE COMMON_SOURCES "common/src/*.cpp" "common/src/**/*.cpp" "common/src/*.hpp" "common/src/**/*.hpp")
file(GLOB_RECURSE SERVER_SOURCES "server/src/*.cpp" "server/src/**/*.cpp" "server/src/*.hpp" "server/src/**/*.hpp")
file(GLOB_RECURSE TRADER_SOURCES "trader/src/*.cpp" "trader/src/**/*.cpp" "trader/src/*.hpp" "trader/src/**/*.hpp")

# Shared (Static) Library
add_library(hft_common STATIC ${COMMON_SOURCES})
set_target_properties(hft_common PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(hft_common PUBLIC 
    common/src 
    common/src/gen 
    common/src/types
)

target_link_libraries(hft_common PRIVATE ${Boost_LIBRARIES} spdlog::spdlog)

# Server Target
add_executable(hft_server ${SERVER_SOURCES})
target_link_libraries(hft_server PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog)
target_include_directories(hft_server PRIVATE 
    common/src
    server/src
    server/src/types
)

# Client Target
add_executable(hft_trader ${TRADER_SOURCES})
target_link_libraries(hft_trader PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog)
target_include_directories(hft_trader PRIVATE 
    common/src 
    trader/src
    trader/src/types
)

add_dependencies(hft_server code_generator)
add_dependencies(hft_trader code_generator)
