cmake_minimum_required(VERSION 3.10)
project(NovaCPP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
endif()

include_directories()

add_executable(NovaCPP src/main.cpp)

if(WIN32)
    target_link_libraries(NovaCPP ws2_32)
else()
    target_link_libraries(NovaCPP pthread)
endif()
