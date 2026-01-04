# Common static library 
include(cmake/code_generator.cmake)

file(GLOB_RECURSE COMMON_SOURCES "src/*.cpp" "src/*.hpp")

list(APPEND COMMON_SOURCES
    ${GEN_DIR_FBS_CPP}/domain_messages_generated.h
    ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.cc
    ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.h
)

add_library(hft_common STATIC ${COMMON_SOURCES})

target_include_directories(hft_common PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/gen 
    ${CMAKE_CURRENT_SOURCE_DIR}/src/types 
    ${LIBPQXX_INCLUDE_DIRS}
    ${RDKAFKA_INCLUDE_DIRS}
    ${Protobuf_INCLUDE_DIRS}
    ${spscqueue_SOURCE_DIR}/include
)

target_link_libraries(hft_common PUBLIC 
    ${Protobuf_LIBRARIES}
    ${LIBPQXX_LIBRARIES}
    ${Boost_LIBRARIES}
    ${RDKAFKAXX_LIB}
    spdlog::spdlog 
    pthread
    iberty 
    atomic
    folly
    glog
    pq 
    dl 
    bfd
)
target_compile_definitions(hft_common PUBLIC SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})

add_dependencies(hft_common sbe_cpp_domain_code_gen)
add_dependencies(hft_common fbs_cpp_domain_code_gen)
add_dependencies(hft_common fbs_cpp_metadata_code_gen)
add_dependencies(hft_common fbs_python_domain_code_gen)
add_dependencies(hft_common proto_cpp_metadata_code_gen)

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
