file(GLOB_RECURSE UNIT_TEST_SOURCES 
    "src/*.cpp"
    "${PROJECT_SOURCE_DIR}/client/src/*.cpp"
    "${PROJECT_SOURCE_DIR}/server/src/*.cpp"
)

list(FILTER UNIT_TEST_SOURCES EXCLUDE REGEX ".*/main.cpp$")

add_executable(run_unit_tests ${UNIT_TEST_SOURCES})

target_link_libraries(run_unit_tests 
    PRIVATE
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
    pthread
    hft_common
)

target_include_directories(run_unit_tests PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/common/src
    ${CMAKE_SOURCE_DIR}/server/src
)

include(GoogleTest)
gtest_discover_tests(run_unit_tests)

add_custom_command(
    TARGET run_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/utest_server_config.ini ${CMAKE_BINARY_DIR}/tests/unit
)

add_custom_command(
    TARGET run_unit_tests POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/config/utest_client_config.ini ${CMAKE_BINARY_DIR}/tests/unit
)

target_compile_definitions(run_unit_tests PRIVATE UNIT_TESTS_BUILD)
