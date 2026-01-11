# cmake/fbs_code_generator.cmake

message(STATUS "--- [FBS] Configuring generator ---")

set(FBS_DOMAIN_SCHEMA ${CMAKE_SOURCE_DIR}/schema/fbs/domain_messages.fbs)

set(GEN_DIR ${CMAKE_BINARY_DIR}/gen)
set(GEN_DIR_FBS_CPP ${GEN_DIR}/fbs/cpp)
set(GEN_DIR_FBS_PYTHON ${GEN_DIR}/fbs/python)

file(MAKE_DIRECTORY ${GEN_DIR_FBS_CPP})
file(MAKE_DIRECTORY ${GEN_DIR_FBS_PYTHON})

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

add_custom_target(fbs_cpp_domain_code_gen DEPENDS ${GEN_DIR_FBS_CPP}/domain_messages_generated.h)
add_custom_target(fbs_python_domain_code_gen DEPENDS ${GEN_DIR_FBS_PYTHON}/domain_messages_generated.py)
