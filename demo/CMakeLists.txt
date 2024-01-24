cmake_minimum_required(VERSION 3.14)

project(demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(demo_cpu demo_cpu.cpp)
target_link_libraries(demo_cpu PRIVATE tinynes)

add_executable(demo_ppu demo_ppu.cpp)
target_link_libraries(demo_ppu PRIVATE tinynes)

add_executable(demo_vscreen demo_vscreen.cpp)
target_link_libraries(demo_vscreen PRIVATE tinynes)