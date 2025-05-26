add_library(crc_lib STATIC src/crc.c)

target_include_directories(crc_lib
    PUBLIC inc # expose crc.h
)