# cmake/code_generator.cmake

set(FBS_DOMAIN_SCHEMA ${CMAKE_SOURCE_DIR}/schema/fbs/domain_messages.fbs)
set(FBS_METADATA_SCHEMA ${CMAKE_SOURCE_DIR}/schema/fbs/metadata_messages.fbs)
set(PROTO_METADATA_SCHEMA ${CMAKE_SOURCE_DIR}/schema/proto/metadata_messages.proto)

set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)
set(GEN_DIR_FBS_CPP ${GEN_DIR}/fbs/cpp)
set(GEN_DIR_FBS_PYTHON ${GEN_DIR}/fbs/python)
set(GEN_DIR_PROTO_CPP ${GEN_DIR}/proto/cpp)

file(MAKE_DIRECTORY ${GEN_DIR_FBS_CPP})
file(MAKE_DIRECTORY ${GEN_DIR_FBS_PYTHON})
file(MAKE_DIRECTORY ${GEN_DIR_PROTO_CPP})

add_custom_command(
    OUTPUT ${GEN_DIR_FBS_CPP}/domain_messages_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${GEN_DIR_FBS_CPP} ${FBS_DOMAIN_SCHEMA}
    DEPENDS ${FBS_DOMAIN_SCHEMA}
    COMMENT "Generating domain messages FlatBuffers code from schema"
)

add_custom_command(
    OUTPUT ${GEN_DIR_FBS_CPP}/metadata_messages_generated.h
    COMMAND flatc --cpp --gen-object-api -o ${GEN_DIR_FBS_CPP} ${FBS_METADATA_SCHEMA}
    DEPENDS ${FBS_METADATA_SCHEMA}
    COMMENT "Generating metadata messages FlatBuffers code from schema"
)

add_custom_command(
    OUTPUT ${GEN_DIR_FBS_PYTHON}/domain_messages_generated.py
    COMMAND flatc --python -o ${GEN_DIR_FBS_PYTHON} ${FBS_DOMAIN_SCHEMA}
    DEPENDS ${FBS_DOMAIN_SCHEMA}
    COMMENT "Generating Python domain messages FlatBuffers code from schema"
)

add_custom_command(
    OUTPUT ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.cc ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.h
    COMMAND ${Protobuf_PROTOC_EXECUTABLE}
    ARGS --cpp_out=${GEN_DIR_PROTO_CPP} -I ${CMAKE_SOURCE_DIR}/schema/proto ${PROTO_METADATA_SCHEMA}
    DEPENDS ${PROTO_METADATA_SCHEMA}
    COMMENT "Generating C++ code from metadata_messages.proto"
)

add_custom_target(fbs_cpp_domain_code_generator DEPENDS ${GEN_DIR_FBS_CPP}/domain_messages_generated.h)
add_custom_target(fbs_cpp_metadata_code_generator DEPENDS ${GEN_DIR_FBS_CPP}/metadata_messages_generated.h)
add_custom_target(fbs_python_domain_code_generator DEPENDS ${GEN_DIR_FBS_PYTHON}/domain_messages_generated.py)
add_custom_target(proto_cpp_metadata_code_generator DEPENDS ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.cc ${GEN_DIR_PROTO_CPP}/metadata_messages.pb.h)
