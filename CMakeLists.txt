cmake_minimum_required(VERSION 3.18)

project(LFIF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_FLAGS "-std=c++20 -Wall -Wextra -pedantic -Wfatal-errors")
set(CMAKE_CXX_FLAGS_DEBUG "-Og -g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -fopenmp -march=native -DNDEBUG -s")

add_subdirectory(liblfif)
add_subdirectory(libppm)
add_subdirectory(tools)
add_subdirectory(extra)
