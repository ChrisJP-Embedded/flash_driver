# Summary

First pass implementation of a high-level flash driver for embedded systems.

Implemented within Docker container / test environment, testing to be driven via python, devloped in WSL.

See main.c, flash_driver_lib/src.

## Project Directory Structure
```
.
|-- CMakeLists.txt
|-- Dockerfile
|-- README.md
|-- crc_lib             <-- Used by flash module to compute CRC32 
|   |-- CMakeLists.txt
|   |-- inc
|   |   `-- crc.h
|   `-- src
|       `-- crc.c
|
|-- file_io_lib         <-- Used by ll_flash_stub to save and load data via file IO
|   |-- CMakeLists.txt
|   |-- inc
|   |   `-- data_persist.h
|   `-- src
|       `-- data_persist.c
|
|-- flash_conf.h        <-- Application flash driver configuration (copied from default in flash_lib)
|-- flash_lib
|   |-- CMakeLists.txt
|   |-- inc
|   |   |-- flash.h
|   |   `-- _flash_conf.h
|   |
|   |-- ll_flash_stub   <-- Stub for register level flash driver
|   |   |-- inc
|   |   |   `-- ll_flash.h
|   |   `-- src
|   |       `-- ll_flash.c
|   `-- src
|       `-- flash.c     <-- High-level flash driver implementation
|
|-- main.c              <-- Simple test harness for flash driver
|  
`-- python
    |-- Pipfile
    `-- flash_test.py   <-- Python test script for flash driver
```