# Common static library 
find_package(folly REQUIRED)
find_package(Glog REQUIRED)
find_package(Protobuf REQUIRED)

# Generate flatbuffers code from the schema
set(DOMAIN_FBS_FILE ${CMAKE_SOURCE_DIR}/schema/fbs/domain_messages.fbs)
set(METADATA_FBS_FILE ${CMAKE_SOURCE_DIR}/schema/fbs/metadata_messages.fbs)
set(METADATA_PROTO_FILE ${CMAKE_SOURCE_DIR}/schema/proto/metadata_messages.proto)

set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)

set(FBS_GEN_DIR ${GEN_DIR}/fbs)
set(PROTO_GEN_DIR ${GEN_DIR}/proto)

file(MAKE_DIRECTORY ${FBS_GEN_DIR})
file(MAKE_DIRECTORY ${PROTO_GEN_DIR})

add_custom_command(
    OUTPUT ${FBS_GEN_DIR}/domain_messages_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${FBS_GEN_DIR} ${DOMAIN_FBS_FILE}
    DEPENDS ${DOMAIN_FBS_FILE}
    COMMENT "Generating domain messages FlatBuffers code from schema"
)
add_custom_command(
    OUTPUT ${FBS_GEN_DIR}/metadata_messages_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${FBS_GEN_DIR} ${METADATA_FBS_FILE}
    DEPENDS ${METADATA_FBS_FILE}
    COMMENT "Generating metadata messages FlatBuffers code from schema"
)
add_custom_command(
    OUTPUT ${PROTO_GEN_DIR}/metadata_messages.pb.cc ${PROTO_GEN_DIR}/metadata_messages.pb.h
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${PROTO_GEN_DIR} -I ${CMAKE_SOURCE_DIR}/schema/proto ${METADATA_PROTO_FILE}
    DEPENDS ${METADATA_PROTO_FILE}
    COMMENT "Generating C++ code from metadata_messages.proto"
)

add_custom_target(fbs_domain_code_generator DEPENDS ${FBS_GEN_DIR}/domain_messages_generated.h)
add_custom_target(fbs_metadata_code_generator DEPENDS ${FBS_GEN_DIR}/metadata_messages_generated.h)
add_custom_target(proto_metadata_code_generator DEPENDS ${PROTO_GEN_DIR}/metadata_messages.pb.cc ${PROTO_GEN_DIR}/metadata_messages.pb.h)

file(GLOB_RECURSE COMMON_SOURCES "src/*.cpp" "src/**/*.cpp" "src/*.hpp" "src/**/*.hpp")

list(APPEND COMMON_SOURCES
    ${FBS_GEN_DIR}/domain_messages_generated.h
    ${FBS_GEN_DIR}/metadata_messages_generated.h
    ${PROTO_GEN_DIR}/metadata_messages.pb.cc
    ${PROTO_GEN_DIR}/metadata_messages.pb.h
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
)

set(SPDLOG_ACTIVE_LEVEL "SPDLOG_LEVEL_ERROR")

target_link_libraries(hft_common PRIVATE 
    ${Boost_LIBRARIES} 
    ${LIBPQXX_LIBRARIES} 
    ${RDKAFKAXX_LIB}
    ${Protobuf_LIBRARIES}
    spdlog::spdlog 
    atomic 
    folly 
    glog 
    pthread 
    dl 
    pq 
    iberty 
)
target_compile_definitions(hft_common PUBLIC SPDLOG_ACTIVE_LEVEL=${SPDLOG_ACTIVE_LEVEL})

add_dependencies(hft_common fbs_domain_code_generator)
add_dependencies(hft_common fbs_metadata_code_generator)
add_dependencies(hft_common proto_metadata_code_generator)

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
