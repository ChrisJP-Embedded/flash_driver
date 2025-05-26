#include "crc.h"
#include "flash.h"
#include "flash_conf.h"

/*
Upper-level flash module, manages app_data layout within flash.

A configurable number of app_data copies, where each app_data copy spans a number of whole pages. 
The active copy is determined by a validity pattern at at its base page addr.
On write we write to the next app_data copy region in ascending sequence, provides
wear-levelling mechanism and also allows ability to roll-back app_data (not yet implemented)

Each app_data copy is prepended with an app_mete_data_t,
which contains: 4BYTE_VALIDATION, 4BYTE_APP_LEN, 4BYTE_CRC

A pointer to the app_data itself and its total size in bytes
are assigned via flash_config_t. Hence app data can be modified
freely at runtime, then a single call to flash_write handles storage..

PUBLIC FUNCTIONS

flash_init:
Takes a flash_config_t pointer (grabs local ptr copy).
Determines if requested layout is valid.
Computes the base address of each app_data copy, stores in array
Identifies active copy via 4BYTE validity pattern in app meta.
Evaluates CRC32 of the currently active copy of flash.
TODO: Roll back through app data copies if corruption detected.

flash_write:
Computes a new app_meta_t CRC32, assigns app_data len + validity (NOT VALID PATTERN, 0xFFFFFFFF)
Grabs the base addr of the next unused app_data copy region from array
Determines number of pages required to store app_data.
Erases the next unused app_data copy region in sequence
Writes the meta_data_t section to the base addr
Writes app_data immediately after meta data.
Reads back and evaluates CRC32 of app_data.
Invalidates the most recent app_data active copy (writes 0 to validity)
Validates the new app_data active copy (writes VALID PATTERN 0x55555555)
*/

// Privates
static struct
{
    flash_config_t* conf_ptr;
    bool has_valid_data;
    bool initialized;

    uint32_t total_flash_bytes;
    uint32_t app_data_total_bytes;
    uint8_t  app_data_active_copy_base_page_idx;
    uint32_t data_copies_base_addrs[CFG_APP_DATA_NUM_COPIES];

} flash;

flash_status_t flash_init(flash_config_t* flash_config_ptr)
{
    assert(flash_config_ptr != NULL);

    if(!flash.initialized)
    {
        flash.conf_ptr = flash_config_ptr;

        flash.initialized = true;

        // Compute total flash we have available in bytes
        for(uint8_t idx = 0; idx < flash.conf_ptr->ll.pages_total_num; ++idx)
        {
            assert(flash.conf_ptr->ll.page_descriptors != NULL);
            flash.total_flash_bytes += flash.conf_ptr->ll.page_descriptors[idx].size_bytes;
        }

        // Compute the total number of bytes required for ALL copies of the app data, check it can fit!
		if((flash.conf_ptr->data_descriptor.data_num_bytes * flash.conf_ptr->num_app_data_copies) > flash.total_flash_bytes)
		{
			return flash_status_total_size_exceeded;
		}
	
        // Compute and store the base address of each copy of the app data
        // While at it read the meta data for any pages which are base of an app data copy
        for(uint32_t copy_num = 0, bytes_traversed = 0, page_dsc_idx = 0; copy_num < flash.conf_ptr->num_app_data_copies; ++copy_num)
        {
            if(copy_num == 0)
            {
                bytes_traversed = 0;
                flash.data_copies_base_addrs[copy_num] = flash.conf_ptr->ll.page_descriptors[page_dsc_idx].base_addr;
                continue;
            }
            else
            {   
                do
                {
                    if((flash.conf_ptr->ll.page_descriptors[page_dsc_idx].size_bytes + bytes_traversed) >= flash.conf_ptr->data_descriptor.data_num_bytes)
                    {
                        flash.data_copies_base_addrs[copy_num] = flash.conf_ptr->ll.page_descriptors[++page_dsc_idx].base_addr;
                        bytes_traversed = 0;
                        break;
                    }
                    else
                    {
                        bytes_traversed += flash.conf_ptr->ll.page_descriptors[page_dsc_idx].size_bytes;
                    }
                } while(page_dsc_idx < flash.conf_ptr->ll.pages_total_num);
            }
        }

        if(ll_flash_init(&flash.conf_ptr->ll) != ll_flash_status_ok)
        {
            return flash_status_ll_init_fault;
        }

        // Search for the current valid copy of app data
        for(uint8_t idx = 0; idx < flash.conf_ptr->num_app_data_copies; ++idx)
        {
            app_data_meta_t app_meta_data = {
                .checksum = 0, 
                .length = 0, 
                .validity = CFG_APP_DATA_INVALID
            };
            
            flash_load_app_data_copy_meta_data(flash.data_copies_base_addrs[idx], &app_meta_data);
            if(app_meta_data.validity == CFG_APP_DATA_VALID)
            {
                if(flash_load_app_data_check_crc(idx, &app_meta_data))
                {
                    flash.conf_ptr->data_descriptor.app_meta_data = app_meta_data;
                    flash.app_data_active_copy_base_page_idx = idx;
                    flash.has_valid_data = true;
                    return flash_status_ok;
                }
                else
                {
                    // where we might try to roll-back app data
                    return flash_status_data_corruption_detected;
                }
            }
        }
    }
    else
    {
        assert(0);
    }

    return flash_status_no_valid_data_found;
}

flash_status_t flash_write(void)
{
    flash_status_t status = flash_status_ok;
    
	if(flash.initialized)
	{
        int32_t num_bytes_to_be_erased;
        uint32_t new_copy_base_page_idx;
        app_data_meta_t new_app_meta_data = {.checksum = 0, .length = 0, .validity = 0};

        // Compute base page idx of the next app_data copy region to be used
        if((flash.app_data_active_copy_base_page_idx + 1) < flash.conf_ptr->num_app_data_copies)
        {
            new_copy_base_page_idx = flash.app_data_active_copy_base_page_idx + 1;
        }
        else
        {
            new_copy_base_page_idx = 0;
        }

        // Erase pages spanned by next app_data copy region
        num_bytes_to_be_erased = flash.conf_ptr->data_descriptor.data_num_bytes;
        for(uint32_t page_idx = new_copy_base_page_idx;; ++page_idx)
        {
            ll_flash_page_erase(page_idx);
            num_bytes_to_be_erased -= flash.conf_ptr->ll.page_descriptors[page_idx].size_bytes;
            if(num_bytes_to_be_erased <= 0)
            {
                break;
            }
        }

        new_app_meta_data.length = flash.conf_ptr->data_descriptor.data_num_bytes;
        new_app_meta_data.validity = CFG_APP_DATA_VALID_CLEAR;
        new_app_meta_data.checksum = crc32(flash.conf_ptr->data_descriptor.data_ptr, 
                                            flash.conf_ptr->data_descriptor.data_num_bytes);

        uint32_t addr = flash.data_copies_base_addrs[new_copy_base_page_idx];
        ll_flash_status_t status = ll_flash_write(addr + sizeof(app_data_meta_t), flash.conf_ptr->data_descriptor.data_ptr,  
                                                    flash.conf_ptr->data_descriptor.data_num_bytes);

        if(status == ll_flash_status_ok)
        {
            status = flash_status_ll_write_fault;
            if(ll_flash_write(addr, (uint8_t*)&new_app_meta_data, sizeof(app_data_meta_t)) == ll_flash_status_ok)
            {
                // Read back the meta data of newly written app_data and eval CRC32
                if(flash_load_app_data_check_crc(new_copy_base_page_idx, &new_app_meta_data))
                {
                    // Invalidate previous app_data copy
                    ll_flash_write(flash.conf_ptr->ll.page_descriptors[flash.app_data_active_copy_base_page_idx].base_addr,
                                            (uint8_t*)&(uint32_t){CFG_APP_DATA_INVALID}, sizeof(uint32_t));

                    // Validate new app_data copy
                    ll_flash_write(flash.conf_ptr->ll.page_descriptors[new_copy_base_page_idx].base_addr,
                                            (uint8_t*)&(uint32_t){CFG_APP_DATA_VALID}, sizeof(uint32_t));

                    flash.app_data_active_copy_base_page_idx = new_copy_base_page_idx;

                    status = ll_flash_status_ok;
                }
            }
        }
	}
	else
	{
		assert(0);
		status = flash_status_uninitialized;
	}

	return status;
}

/******************/
/**** PRIVATES ****/
/******************/

static bool flash_load_app_data_copy_meta_data(uint32_t copy_base_addr, app_data_meta_t* meta_data)
{
    // Reads and returns app meta data from base addr of an app data copy range
    // Typically used when searching for currently valid app data copy

    ll_flash_read(copy_base_addr, (uint8_t*)meta_data, sizeof(app_data_meta_t));
    return true;
}

/*
    Loads app_data_meta_t from flash (which holds app_data length and CRC32).
    Computes CRC32 over the app_data range and compares against that read from flash.
*/
static bool flash_load_app_data_check_crc(uint8_t page_idx, app_data_meta_t * app_data_meta)
{
    uint32_t base_addr = flash.conf_ptr->ll.page_descriptors[page_idx].base_addr;

    // Read out meta data from base of specified page 
    ll_flash_read(base_addr + sizeof(app_data_meta_t), flash.conf_ptr->data_descriptor.data_ptr, 
                    flash.conf_ptr->data_descriptor.data_num_bytes);

    // Compute CRC32 for the app_data range 
    uint32_t _crc32 = crc32(flash.conf_ptr->data_descriptor.data_ptr, 
                                flash.conf_ptr->data_descriptor.data_num_bytes);

    // Compare crc computed from read app data against the meta copy
    if (_crc32 == app_data_meta->checksum)
    {
        return true;
    }

    return false;
}