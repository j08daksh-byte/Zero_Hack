cmake_minimum_required(VERSION 3.10)
project(NovaCPP CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601 WIN32_LEAN_AND_MEAN)
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

add_executable(NovaCPP 
    src/main.cpp 
    src/frontend/App.cpp 
    src/backend/Database.cpp 
    src/backend/Auth.cpp
)

if(WIN32)
    target_link_libraries(NovaCPP ws2_32)
else()
    target_link_libraries(NovaCPP pthread)
endif()

# Test Suite Executable
add_executable(nova_tests tests/test_main.cpp)
if(WIN32)
    target_link_libraries(nova_tests ws2_32)
else()
    target_link_libraries(nova_tests pthread)
endif()

# Custom target to open the web app in the default browser
if(WIN32)
    add_custom_target(run-html
        COMMAND cmd.exe /c start "" "http://localhost:8080"
        COMMENT "Opening http://localhost:8080 in the default browser..."
    )
endif()
