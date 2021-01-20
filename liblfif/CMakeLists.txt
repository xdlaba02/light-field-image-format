cmake_minimum_required(VERSION 3.18)

file(GLOB SRC_FILES CONFIGURE_DEPENDS src/*.cc)

add_library(lfif STATIC SHARED ${SRC_FILES})

target_include_directories(lfif PUBLIC include)
