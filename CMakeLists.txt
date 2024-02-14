cmake_minimum_required(VERSION 3.18)
project(cachesim C)

set(CMAKE_C_STANDARD 99)

add_executable(cachesim main.c
        cache.c
        cache.h
        main.h)
