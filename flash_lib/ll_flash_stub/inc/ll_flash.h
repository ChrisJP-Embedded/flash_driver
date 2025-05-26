#ifndef LL_FLASH_H
#define LL_FLASH_H

#include <stdint.h>

typedef struct 
{
    // Assign to externs from
    // linker script values
    uint32_t base_addr;
    uint32_t size_bytes;
} page_dsc_t;

typedef enum
{
    write_size_8bit,
    write_size_16bit,
    write_size_32bit,
    write_size_64bit,
    write_size_128bit,
} flash_write_size_t;

typedef struct 
{
    uint8_t num_flash_keys;
    uint32_t pages_total_num;
    flash_write_size_t write_granularity;
    const uint32_t* flash_keys;
    const page_dsc_t* page_descriptors;
} ll_flash_config_t;

typedef enum
{
    ll_flash_status_ok,
} ll_flash_status_t;

ll_flash_status_t ll_flash_init(ll_flash_config_t* _ll_flash_ptr);
ll_flash_status_t ll_flash_write(uint32_t addr, uint8_t* data, uint32_t size);
ll_flash_status_t ll_flash_read(uint32_t addr, uint8_t* data, uint32_t size);
ll_flash_status_t ll_flash_page_erase(uint8_t page_idx);

#endif