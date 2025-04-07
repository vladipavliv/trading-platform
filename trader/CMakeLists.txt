# Trader Target
file(GLOB_RECURSE TRADER_SOURCES "src/*.cpp" "src/**/*.cpp" "src/*.hpp" "src/**/*.hpp")

add_executable(hft_trader ${TRADER_SOURCES})
target_compile_definitions(hft_trader PRIVATE)

target_link_libraries(hft_trader PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES} atomic)

target_include_directories(hft_trader PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/common/src
)

# mimalloc
if(USE_MIMALLOC)
    find_package(mimalloc REQUIRED)
    target_link_libraries(hft_trader PRIVATE mimalloc)
endif()

add_dependencies(hft_trader fbs_code_generator)

add_custom_command(
    TARGET hft_trader POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/trader_config.ini ${CMAKE_BINARY_DIR}/trader
)
