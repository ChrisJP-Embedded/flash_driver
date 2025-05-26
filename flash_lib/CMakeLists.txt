add_library(flash_lib STATIC
    src/flash.c
    ll_flash_stub/src/ll_flash.c
)

# includes 
target_include_directories(flash_lib
    PUBLIC  inc
    PUBLIC  ll_flash_stub/inc
)

# flash_lib depends on crc_lib (and needs its headers)
target_link_libraries(flash_lib PUBLIC crc_lib file_io_lib)
