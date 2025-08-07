cmake_minimum_required(VERSION 3.20)
project(gameboy)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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

find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(IMGUI REQUIRED imgui)

target_link_libraries(gameboy
    PRIVATE
        SDL2::SDL2
        SDL2::SDL2main
        ${IMGUI_LIBRARIES}
        OpenGL::GL
)

target_include_directories(gameboy PRIVATE
    ${IMGUI_INCLUDE_DIRS}
)

target_compile_options(gameboy PRIVATE ${IMGUI_CFLAGS_OTHER})
target_link_libraries(gameboy PRIVATE ${IMGUI_LDFLAGS_OTHER})

