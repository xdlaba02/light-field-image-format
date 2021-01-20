cmake_minimum_required(VERSION 3.18)

add_library(ppm STATIC SHARED src/ppm.cc)

target_include_directories(ppm PUBLIC include)
