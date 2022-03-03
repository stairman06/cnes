cmake_minimum_required(VERSION 3.0)

project(cnes)

add_compile_options(-Weverything -Wno-gnu-binary-literal)

include_directories(src)

set(SOURCE_FILES
    src/main.c
    src/bus.c
    src/cpu.c
    src/util.c
    src/debug.c)

add_executable(cnes ${SOURCE_FILES})
