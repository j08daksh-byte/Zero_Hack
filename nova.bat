@echo off
setlocal enabledelayedexpansion

if not exist build mkdir build

if "%1"=="test" (
    echo Building and running test suite...
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. tests/test_main.cpp -lws2_32 -o build\nova_tests.exe
    if %errorlevel% equ 0 (
        build\nova_tests.exe
    )
) else if "%1"=="run" (
    echo Building NovaCPP...
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. src/main.cpp src/frontend/App.cpp src/backend/Database.cpp src/backend/Auth.cpp -lws2_32 -o build\NovaCPP.exe
    if %errorlevel% equ 0 (
        start http://localhost:8080
        build\NovaCPP.exe
    )
) else if "%1"=="build" (
    echo Building NovaCPP and Tests...
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. src/main.cpp src/frontend/App.cpp src/backend/Database.cpp src/backend/Auth.cpp -lws2_32 -o build\NovaCPP.exe
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. tests/test_main.cpp -lws2_32 -o build\nova_tests.exe
    if %errorlevel% equ 0 (
        echo Build complete. Executables in build\
    )
) else if "%1"=="clean" (
    if exist build rmdir /s /q build
    echo Cleaned build directory.
) else (
    echo Building NovaCPP and Tests...
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. src/main.cpp src/frontend/App.cpp src/backend/Database.cpp src/backend/Auth.cpp -lws2_32 -o build\NovaCPP.exe
    g++ -std=c++17 -D_WIN32_WINNT=0x0601 -I. tests/test_main.cpp -lws2_32 -o build\nova_tests.exe
    echo Build complete.
    echo Usage:
    echo   nova build - Build web server and test suite
    echo   nova run   - Build and start web server
    echo   nova test  - Build and execute test suite
)
