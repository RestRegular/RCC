cmake_minimum_required(VERSION 3.20)
project(rio_runtime C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")

add_library(rio_runtime SHARED
    src/rio_object.c
    src/rio_string.c
    src/rio_gc.c
    src/rio_print.c
    src/rio_runtime.c
)

target_include_directories(rio_runtime PUBLIC include)
