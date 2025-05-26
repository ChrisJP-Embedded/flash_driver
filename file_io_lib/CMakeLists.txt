add_library(file_io_lib STATIC
    src/file_io.c
)

# headers needed while *building* flash_lib
target_include_directories(file_io_lib
    PUBLIC  inc 
)
