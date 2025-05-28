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
    uint32_t checksum;
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
} flash_status_t;

typedef struct 
{
	uint32_t data_num_bytes;
	uint8_t * data_ptr;
    app_data_meta_t app_meta_data;
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

static bool flash_load_app_data_check_crc(uint8_t page_idx, app_data_meta_t* meta_data);
static bool flash_load_app_data_copy_meta_data(uint32_t copy_base_addr, app_data_meta_t* meta_data);

#endif