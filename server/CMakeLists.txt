# Server Target
file(GLOB_RECURSE SERVER_SOURCES "src/*.cpp" "src/**/*.cpp" "src/*.hpp" "src/**/*.hpp")

add_executable(hft_server ${SERVER_SOURCES})

target_compile_definitions(hft_server PRIVATE)
target_link_libraries(hft_server PRIVATE hft_common ${Boost_LIBRARIES} spdlog::spdlog ${LIBPQXX_LIBRARIES} atomic)

target_include_directories(hft_server PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src 
    ${CMAKE_SOURCE_DIR}/common/src
)

# mimalloc
if(USE_MIMALLOC)
    find_package(mimalloc REQUIRED)
    target_link_libraries(hft_server PRIVATE mimalloc)
endif()

add_custom_command(
    TARGET hft_server POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/server_config.ini ${CMAKE_BINARY_DIR}/server
)
