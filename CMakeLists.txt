cmake_minimum_required(VERSION 3.30)
project(hashmap C)

set(CMAKE_C_STANDARD 11)

add_library(hashmap STATIC
        hashmap.c
        hashmap.h)
