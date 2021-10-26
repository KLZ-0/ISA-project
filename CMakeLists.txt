cmake_minimum_required(VERSION 3.10)
project(mytftpclient)

set(CMAKE_C_STANDARD 99)

add_executable(mytftpclient main.c client.c)
