cmake_minimum_required(VERSION 3.18)

set(COMPRESS_SOURCES   src/compress.cc   src/lfif4d_compress.cc   src/plenoppm.cc src/file_mask.cc)
set(DECOMPRESS_SOURCES src/decompress.cc src/lfif4d_decompress.cc src/plenoppm.cc src/file_mask.cc)

add_executable(lfif4d_compress ${COMPRESS_SOURCES})
add_executable(lfif4d_decompress ${DECOMPRESS_SOURCES})

target_include_directories(lfif4d_compress   PUBLIC include)
target_include_directories(lfif4d_decompress PUBLIC include)

target_link_libraries(lfif4d_compress   lfif ppm)
target_link_libraries(lfif4d_decompress lfif ppm)
