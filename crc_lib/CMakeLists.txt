add_library(crc_lib STATIC src/crc.c)

#includes
target_include_directories(crc_lib
    PUBLIC inc
)