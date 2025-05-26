add_library(file_io_lib STATIC
    src/file_io.c
)

#includes
target_include_directories(file_io_lib
    PUBLIC  inc 
)
