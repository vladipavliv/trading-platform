# Common static library 
file(GLOB_RECURSE COMMON_SOURCES CONFIGURE_DEPENDS "src/*.cpp")

if(SERIALIZATION STREQUAL "SBE" OR BUILD_TESTS OR BUILD_BENCHMARKS)
    include(cmake/sbe_code_generator.cmake)
endif()

if(SERIALIZATION STREQUAL "FBS" OR BUILD_TESTS OR BUILD_BENCHMARKS)
    include(cmake/fbs_code_generator.cmake)
    list(APPEND COMMON_SOURCES
        ${GEN_DIR_FBS_CPP}/domain_messages_generated.h
    )
endif()

add_library(hft_common STATIC ${COMMON_SOURCES})

target_include_directories(hft_common PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gen 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/types 
    ${LIBPQXX_INCLUDE_DIRS}
    ${RDKAFKA_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${CMAKE_BINARY_DIR}/gen
)

target_link_libraries(hft_common PUBLIC 
    ${Protobuf_LIBRARIES}
    ${LIBPQXX_LIBRARIES}
    ${Boost_LIBRARIES}
    ${RDKAFKAXX_LIB}
    spdlog::spdlog 
    pthread
    pq 
    atomic
    iberty 
    folly
    glog
    dl 
    bfd
)
target_compile_definitions(hft_common PUBLIC SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})

if (BUILD_TESTS OR BUILD_BENCHMARKS)
    add_dependencies(hft_common sbe_cpp_domain_code_gen)
    add_dependencies(hft_common fbs_cpp_domain_code_gen)
    add_dependencies(hft_common fbs_python_domain_code_gen)
elseif(SERIALIZATION STREQUAL "SBE")
    add_dependencies(hft_common sbe_cpp_domain_code_gen)
elseif(SERIALIZATION STREQUAL "FBS")
    add_dependencies(hft_common fbs_cpp_domain_code_gen)
    add_dependencies(hft_common fbs_python_domain_code_gen)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT IPO_SUPPORTED OUTPUT IPO_OUTPUT)
if(IPO_SUPPORTED)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
