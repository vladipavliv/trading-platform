# Client Target
file(GLOB_RECURSE CLIENT_SOURCES "src/*.cpp")

add_executable(hft_client ${CLIENT_SOURCES})
target_compile_definitions(hft_client PRIVATE)

target_link_libraries(hft_client PRIVATE 
    hft_common 
    ${Boost_LIBRARIES} 
    ${LIBPQXX_LIBRARIES} 
    spdlog::spdlog 
    atomic
    iberty
    bfd
)

target_include_directories(hft_client PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/common/src
)

add_custom_command(
    TARGET hft_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/client_config.ini ${CMAKE_BINARY_DIR}/client
)
