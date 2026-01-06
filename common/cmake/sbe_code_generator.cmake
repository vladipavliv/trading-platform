# cmake/sbe_code_generator.cmake

message(STATUS "--- [SBE] Configuring generator ---")

set(SBE_TOOL_JAR /usr/local/bin/sbe-tool.jar)

set(SBE_DOMAIN_SCHEMA ${CMAKE_SOURCE_DIR}/schema/sbe/domain_messages.xml)

set(GEN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src/gen)
set(GEN_DIR_SBE_CPP ${GEN_DIR}/sbe/cpp)

file(MAKE_DIRECTORY ${GEN_DIR_SBE_CPP})

set(SBE_GENERATED_FILES
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/Char4.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/Char32.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/LoginRequest.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/LoginResponse.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/Message.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/MessageHeader.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/OrderAction.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/Order.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/OrderState.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/OrderStatus.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/TickerPrice.h
  ${GEN_DIR_SBE_CPP}/hft_serialization_gen_sbe_domain/TokenBindRequest.h
)

add_custom_command(
  OUTPUT ${SBE_GENERATED_FILES}
  COMMAND java -Dsbe.target.language=CPP -Dsbe.output.dir=${GEN_DIR_SBE_CPP} -jar ${SBE_TOOL_JAR} ${SBE_DOMAIN_SCHEMA}
  DEPENDS ${SBE_DOMAIN_SCHEMA} ${SBE_TOOL_JAR}
  COMMENT "Generating SBE domain messages code from schema"
)

add_custom_target(sbe_cpp_domain_code_gen ALL DEPENDS ${SBE_GENERATED_FILES})
