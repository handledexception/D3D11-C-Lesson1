cmake_minimum_required(VERSION 3.10)

project(c-d3d11-lesson1)

set(TARGET_COMPILE_DEFS
    -DUNICODE
    -D_UNICODE
    -DWIN32_LEAN_AND_MEAN)

add_executable(d3d11_lesson1 WIN32 main.c)
target_compile_definitions(d3d11_lesson1 PRIVATE ${TARGET_COMPILE_DEFS})
