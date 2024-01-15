cmake_minimum_required(VERSION 3.14)

project(tinynes LANGUAGES CXX)

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

# Add sources
file(GLOB TINYNES_SOURCES
    ${PROJECT_SOURCE_DIR}/main.cpp
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)


# Set static if BUILD_STATIC is set
if (BUILD_STATIC)
    set(SFML_STATIC_LIBRARIES TRUE)
    # Link libgcc and libstc++ statically as well
    if(CMAKE_COMPILER_IS_GNUCXX)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++ -static-libgcc")
    endif()
endif()

# SFML
find_package(SFML 2 REQUIRED COMPONENTS audio graphics window system)
if(NOT SFML_FOUND)
    message(STATUS "Make sure the SFML libraries exist.")
endif()

# spdlog
find_package(spdlog REQUIRED)

add_executable(tinynes ${TINYNES_SOURCES})
target_include_directories(tinynes PUBLIC "${PROJECT_SOURCE_DIR}/include" ${SFML_INCLUDE_DIR})
target_link_libraries(tinynes PRIVATE ${SFML_LIBRARIES} ${SFML_DEPENDENCIES} spdlog::spdlog_header_only)
set_target_properties(
    tinynes PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON)