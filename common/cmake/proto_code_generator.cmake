# cmake/proto_code_generator.cmake

message(STATUS "--- [PROTO] Configuring generator ---")

set(PROTO_METADATA_SCHEMA ${CMAKE_SOURCE_DIR}/schema/proto/metadata_messages.proto)

set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)
set(GEN_DIR_PROTO_CPP ${GEN_DIR}/proto/cpp)

file(MAKE_DIRECTORY ${GEN_DIR_PROTO_CPP})

add_custom_command(
    OUTPUT ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.cc ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.h
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${GEN_DIR_PROTO_CPP} -I ${CMAKE_SOURCE_DIR}/schema/proto ${PROTO_METADATA_SCHEMA}
    DEPENDS ${PROTO_METADATA_SCHEMA}
    COMMENT "Generating C++ code from metadata_messages.proto"
)

add_custom_target(proto_cpp_metadata_code_gen DEPENDS ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.cc ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.h)
