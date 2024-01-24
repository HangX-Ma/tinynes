cmake_minimum_required(VERSION 3.14)

project(test LANGUAGES CXX)

set(TEST_FILES)

foreach(TEST_FILE ${TEST_FILES})
    get_filename_component(OUT_BINARY_NAME ${TEST_FILE} NAME_WE)
    add_executable(${OUT_BINARY_NAME} ${TEST_FILE})
    target_link_libraries(${OUT_BINARY_NAME} PRIVATE
        ${CMAKE_THREAD_LIBS_INIT}
        GTest::GTest
        GTest::Main
    )
    gtest_discover_tests(${OUT_BINARY_NAME})
    message(STATUS "GTest add test file '${OUT_BINARY_NAME}'")
endforeach()