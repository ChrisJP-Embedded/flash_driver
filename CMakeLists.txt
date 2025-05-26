cmake_minimum_required(VERSION 3.18)
project(c_project C)

# --- sub‑libraries ---
add_subdirectory(crc_lib)
add_subdirectory(flash_lib)
add_subdirectory(data_persist_lib)

# --- application ---
add_executable(${PROJECT_NAME} main.c)
# flash_lib already links crc_lib, so we only link flash_lib here
target_link_libraries(${PROJECT_NAME} PRIVATE flash_lib)
