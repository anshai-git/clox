cmake_minimum_required(VERSION 4.1)
project(clox C)

set(CMAKE_C_STANDARD 11)

include_directories(.)

add_executable(clox
        chunk.c
        chunk.h
        common.h
        compiler.c
        compiler.h
        debug.c
        debug.h
        main.c
        memory.c
        memory.h
        object.c
        object.h
        scanner.c
        scanner.h
        value.c
        value.h
        vm.c
        vm.h)
