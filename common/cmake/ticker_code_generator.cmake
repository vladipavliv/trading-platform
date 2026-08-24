# cmake/ticker_code_generator.cmake

message(STATUS "--- [TICKER] Configuring generator ---")

# Пути
set(DATA_JSON_SOURCE ${CMAKE_SOURCE_DIR}/data/data.json)  # Исходный/генерируемый файл
set(TICKER_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/generate_ticker_map.py)
set(DATA_GENERATOR_SCRIPT ${CMAKE_SOURCE_DIR}/scripts/generate_tickers_json.py)

# --- Генерируем data.json в исходниках (если его нет) ---
set(TICKER_COUNT 10 CACHE STRING "Number of tickers to generate")

add_custom_command(
    OUTPUT ${DATA_JSON_SOURCE}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_SOURCE_DIR}/data
    COMMAND ${Python3_EXECUTABLE} ${DATA_GENERATOR_SCRIPT}
            ${TICKER_COUNT}
            -o ${DATA_JSON_SOURCE}
    COMMENT "Generating data.json with ${TICKER_COUNT} tickers"
    VERBATIM
)

# --- Генерация ticker_map.hpp (зависит от data.json) ---
set(GEN_DIR ${CMAKE_BINARY_DIR}/gen)
set(GEN_DIR_TICKER ${GEN_DIR}/ticker)
file(MAKE_DIRECTORY ${GEN_DIR_TICKER})

set(TICKER_GENERATED_HPP ${GEN_DIR_TICKER}/ticker_map.hpp)
set(TICKER_GENERATED_FILES ${TICKER_GENERATED_HPP})

add_custom_command(
    OUTPUT ${TICKER_GENERATED_FILES}
    COMMAND ${Python3_EXECUTABLE} ${TICKER_SCRIPT}
            --input ${DATA_JSON_SOURCE}
            --output-hpp ${TICKER_GENERATED_HPP}
    DEPENDS ${DATA_JSON_SOURCE} ${TICKER_SCRIPT}
    COMMENT "Generating ticker_map.hpp from data.json"
    VERBATIM
)

# --- Копируем data.json в билд-директорию для рантайма ---
set(RUNTIME_DATA_JSON ${CMAKE_BINARY_DIR}/data.json)

add_custom_command(
    OUTPUT ${RUNTIME_DATA_JSON}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DATA_JSON_SOURCE}
        ${RUNTIME_DATA_JSON}
    DEPENDS ${DATA_JSON_SOURCE}
    COMMENT "Copying data.json to build directory for runtime"
    VERBATIM
)

# --- Общая цель ---
add_custom_target(ticker_code_gen ALL 
    DEPENDS ${TICKER_GENERATED_FILES} ${RUNTIME_DATA_JSON}
)

# Экспортируем пути
set(TICKER_GENERATED_HPP ${TICKER_GENERATED_HPP} PARENT_SCOPE)
set(TICKER_GENERATED_FILES ${TICKER_GENERATED_FILES} PARENT_SCOPE)
set(TICKER_GEN_DIR ${GEN_DIR_TICKER} PARENT_SCOPE)

message(STATUS "--- [TICKER] Generator configured ---")
message(STATUS "  data.json source: ${DATA_JSON_SOURCE}")
message(STATUS "  ticker_map.hpp:   ${TICKER_GENERATED_HPP}")
message(STATUS "  data.json copy:   ${RUNTIME_DATA_JSON}")