add_library(flash_lib STATIC
    src/flash.c
    ll_flash_stub/src/ll_flash.c
)

# headers needed while *building* flash_lib
target_include_directories(flash_lib
    PUBLIC  inc                 # flash.h visible to users (main.c)
    PUBLIC  ll_flash_stub/inc   # ll_flash.h only for internal use
)

# flash_lib depends on crc_lib (and needs its headers)
target_link_libraries(flash_lib PUBLIC crc_lib file_io_lib)
