#ifndef FLASH_H
#define FLASH_H

#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "ll_flash.h"

typedef struct
{
    uint32_t validity;
    uint32_t length;
    uint32_t crc32;
} app_data_meta_t;

typedef enum
{
    flash_status_ok,
    flash_status_uninitialized,
	flash_status_total_size_exceeded,
	flash_status_no_valid_data_found,
    flash_status_data_corruption_detected,
    flash_status_crc_check_failure,
    flash_status_ll_init_fault,
    flash_status_ll_write_fault,
    flash_status_ll_read_fault,
    flash_status_ll_erase_fault,
} flash_status_t;

typedef union
{
    uint8_t  * byte_ptr; 
    uint32_t * word_ptr;
    uint16_t * half_word_ptr;
} app_data_t;


typedef struct __attribute__((packed)) 
{
	uint32_t data_num_bytes;
    app_data_meta_t _app_data_meta;
    uint8_t* app_data;
} flash_data_dsc_t;

typedef struct
{
    bool has_valid_data;
    bool initialized;
    uint8_t num_app_data_copies;
	uint8_t pages_per_app_data_copy;
    uint32_t total_num_bytes_of_flash;
	flash_data_dsc_t data_descriptor;
    ll_flash_config_t ll;

} flash_config_t;

flash_status_t flash_init(flash_config_t* flash_config_ptr);
flash_status_t flash_write(void);
flash_status_t flash_read(void);

static uint32_t flash_app_data_bytes_inc_meta(void);

static bool flash_load_app_data_and_check_crc(uint32_t page_idx, app_data_meta_t * app_data_meta);
static bool flash_read_copy_meta_data(uint32_t copy_idx, app_data_meta_t* app_meta_data);

#endif