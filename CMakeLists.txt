cmake_minimum_required(VERSION 3.0)

project(cnes)

add_compile_options(-Weverything)

include_directories(src)

set(SOURCE_FILES
    src/main.c
    src/bus.c
    src/cpu.c)

add_executable(cnes ${SOURCE_FILES})
