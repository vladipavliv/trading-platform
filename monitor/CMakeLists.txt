# Server Target
file(GLOB_RECURSE MONITOR_SOURCES "src/*.cpp" "src/**/*.cpp" "src/*.hpp" "src/**/*.hpp")

add_executable(hft_monitor ${MONITOR_SOURCES})

target_compile_definitions(hft_monitor PRIVATE)
target_link_libraries(hft_monitor PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog atomic)

target_include_directories(hft_monitor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_SOURCE_DIR}/common/src
)

add_custom_command(
    TARGET hft_monitor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/monitor_config.ini ${CMAKE_BINARY_DIR}/monitor
)
