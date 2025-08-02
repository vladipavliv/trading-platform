file(GLOB_RECURSE COMMON_TEST_SOURCES 
    "src/*.cpp" 
    "src/**/*.cpp"
)

add_executable(test_common_unit ${COMMON_TEST_SOURCES})

target_link_libraries(test_common_unit 
    PRIVATE
    GTest::gtest
    GTest::gmock
    GTest::gtest_main
    pthread
    hft_common
)

include(GoogleTest)
gtest_discover_tests(test_common_unit)