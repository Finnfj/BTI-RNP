cmake_minimum_required(VERSION 3.12)
project(c_sockets_chat C)

set(CMAKE_C_STANDARD 11)


file(GLOB TUTORIAL
        src/*.h
        src/*.c)
add_executable(c_sockets_chat src/main.c ${TUTORIAL})