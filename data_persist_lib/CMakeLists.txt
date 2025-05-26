add_library(data_persist_lib STATIC
    src/data_persist.c
)

# headers needed while *building* flash_lib
target_include_directories(data_persist_lib
    PUBLIC  inc 
)
