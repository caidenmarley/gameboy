cmake_minimum_required(VERSION 3.20)
project(gameboy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# enable warnings
if(MSVC)
    add_compile_options(/W4 /permissive-)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

include_directories(include)

file(GLOB_RECURSE SRC_FILES
    src/*.cpp
)

# create main exe, cmake --build build --target stock_bot
add_executable(gameboy main.cpp ${SRC_FILES})
