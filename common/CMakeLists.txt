# Common static library 
file(GLOB_RECURSE COMMON_SOURCES "src/*.cpp" "src/**/*.cpp" "src/*.hpp" "src/**/*.hpp")
add_library(hft_common STATIC ${COMMON_SOURCES})

find_package(folly REQUIRED)
find_package(Glog REQUIRED)

target_include_directories(hft_common PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gen 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/types 
    ${LIBPQXX_INCLUDE_DIRS}
)

set(SPDLOG_ACTIVE_LEVEL "SPDLOG_LEVEL_ERROR")

target_link_libraries(hft_common PRIVATE ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES} atomic folly glog pthread dl iberty)
target_compile_definitions(hft_common PUBLIC SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})

# Generate flatbuffers code from the schema
set(SCHEMA_FILE ${CMAKE_CURRENT_SOURCE_DIR}/schema/marketdata.fbs)
set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)

add_custom_command(
    OUTPUT ${GEN_DIR}/marketdata_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${GEN_DIR} ${SCHEMA_FILE}
    DEPENDS ${SCHEMA_FILE}
    COMMENT "Generating FlatBuffers code from schema"
)
add_custom_target(fbs_code_generator DEPENDS ${GEN_DIR}/marketdata_generated.h)
add_dependencies(hft_common fbs_code_generator)

# Make library
set(CMAKE_AR "/usr/bin/ar")
set(CMAKE_RANLIB "/usr/bin/ranlib")

add_custom_command(TARGET hft_common POST_BUILD
    COMMAND ranlib ${CMAKE_CURRENT_BINARY_DIR}/libhft_common.a
    COMMENT "Running ranlib on libhft_common.a"
)

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
