cmake_minimum_required(VERSION 3.14)

project(tinynes LANGUAGES CXX)

if(PROJECT_SOURCE_DIR STREQUAL PROJECT_BINARY_DIR)
message(FATAL_ERROR "In-source builds are not allowed")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose Release or Debug" FORCE)
endif()

set(BUILD_STATIC FALSE CACHE STRING "Set this to link external libraries statically")

if(CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -g")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")
endif()

# generate compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# =================================
#              libtinynes
# =================================

# Add sources
set(TINYNES_SOURCES
    ${CMAKE_SOURCE_DIR}/src/apu.cpp
    ${CMAKE_SOURCE_DIR}/src/bus.cpp
    ${CMAKE_SOURCE_DIR}/src/cpu.cpp
    ${CMAKE_SOURCE_DIR}/src/ppu.cpp
    ${CMAKE_SOURCE_DIR}/src/cartridge.cpp
    ${CMAKE_SOURCE_DIR}/src/vsound.cpp
    ${CMAKE_SOURCE_DIR}/src/mappers/mapper000.cpp
)

# SFML
find_package(SFML 2 REQUIRED COMPONENTS audio graphics window system)
if(NOT SFML_FOUND)
    message(STATUS "Make sure the SFML libraries exist.")
endif()
# Set static if BUILD_STATIC is set
if (BUILD_STATIC)
    set(SFML_STATIC_LIBRARIES TRUE)
    # Link libgcc and libstc++ statically as well
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
    endif()
endif()

# spdlog
find_package(spdlog REQUIRED)

# tinynes library
add_library(tinynes STATIC ${TINYNES_SOURCES})
target_include_directories(tinynes PUBLIC "${CMAKE_SOURCE_DIR}/include" ${SFML_INCLUDE_DIR})
target_link_libraries(tinynes PRIVATE ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} spdlog::spdlog_header_only)
set_target_properties(
    tinynes PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)
target_compile_definitions(tinynes
    PUBLIC "TINYNES_WORKSPACE=\"${CMAKE_SOURCE_DIR}\"")

# =================================
#              Demo
# =================================
add_subdirectory(demo)

# =================================
#              Test
# =================================
# GTest
find_package(Threads REQUIRED)
find_package(GTest REQUIRED)
if (GTest_FOUND)
    message(STATUS "Packaged version of GTest will be used.")
else()
    message(STATUS "Bundled version of GTest will be downloaded and used.")
    include(FetchContent)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-v1.14.0
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/extern/gtest
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
    add_library(GTest::GTest INTERFACE IMPORTED)
    target_link_libraries(GTest::GTest INTERFACE gtest_main)
endif()

# CTest
enable_testing()
include_directories(${GTEST_INCLUDE_DIRS})
add_subdirectory(test)