cmake_minimum_required(VERSION 3.15)

project(MouseLogger
    VERSION 1.0
    LANGUAGES C
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

add_executable(MouseLogger
    MouseLogger/main.c
)

target_include_directories(MouseLogger PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/MouseLogger
)

# Use Windows subsystem instead of console
if (MSVC)
    target_link_options(MouseLogger PRIVATE /SUBSYSTEM:WINDOWS)
    target_compile_options(MouseLogger PRIVATE /W4)
else()
    target_compile_options(MouseLogger PRIVATE -Wall -Wextra -Wpedantic)
endif()
