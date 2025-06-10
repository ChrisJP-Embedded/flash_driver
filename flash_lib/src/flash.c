#include <assert.h>
#include "crc.h"
#include "flash.h"
#include "flash_conf.h"

/* Upper-level flash module, manages app_data layout within flash.
   A configurable number of app_data copies, where each app_data copy spans a number of whole pages.
   The active copy is determined by a validity pattern at at its base page addr.
   On write we write to the next app_data copy region in ascending sequence, provides wear-levelling mechanism and also allows ability to roll-back app_data (not yet implemented)

   Each app_data copy is prepended with an app_mete_data_t, which contains:
   4BYTE_VALIDATION, 4BYTE_APP_LEN, 4BYTE_CRC

   A pointer to the app_data itself and its total size in bytes are assigned via flash_config_t.
   Hence app data can be modified freely at runtime, then a single call to flash_write handles storage..

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
       Validates the new app_data active copy (writes VALID PATTERN 0x55555555) */

// Privates
static struct {
    flash_config_t* conf_ptr;
    bool has_valid_data;
    bool initialized;

    uint32_t total_flash_bytes;
    uint8_t  app_data_active_copy_base_page_idx;
    uint32_t data_copies_base_addrs[CFG_APP_DATA_NUM_COPIES];

} flash;

/********************************/
/*          flash_init          */
/********************************/
flash_status_t flash_init(flash_config_t* flash_config_ptr)
{
    /*--------- Sanity checks on arguments ---------*/
    assert(flash_config_ptr != NULL);
    assert(flash_config_ptr->ll.page_descriptors != NULL);
    assert(flash_config_ptr->num_app_data_copies == CFG_APP_DATA_NUM_COPIES);
    assert(flash_config_ptr->data_descriptor.app_data != NULL);
    assert(flash_config_ptr->data_descriptor.data_num_bytes > 0);
    assert(flash_config_ptr->ll.pages_total_num > 0);                       /* Added */

    /* Each page must have a non‑zero size */
    for (uint8_t i = 0; i < flash_config_ptr->ll.pages_total_num; ++i)
    {
        assert(flash_config_ptr->ll.page_descriptors[i].size_bytes > 0);     /* Added */
    }

    /*------------------------------------------------*/
    if (!flash.initialized)
    {
        flash.conf_ptr = flash_config_ptr;
        flash.initialized = true;

        /* Compute total flash we have available in bytes */
        flash.total_flash_bytes = 0;
        for (uint8_t idx = 0; idx < flash.conf_ptr->ll.pages_total_num; ++idx)
        {
            assert(flash.conf_ptr->ll.page_descriptors != NULL);
            flash.total_flash_bytes += flash.conf_ptr->ll.page_descriptors[idx].size_bytes;
        }
        assert(flash.total_flash_bytes > 0);                                /* Added */

        /* Ensure requested copies fit in the physical flash */
        uint32_t total_required = flash_app_data_bytes_inc_meta() * flash.conf_ptr->num_app_data_copies;
        assert(total_required > 0);                                          /* Added */

        if (total_required > flash.total_flash_bytes)
        {
            return flash_status_total_size_exceeded;
        }

        /* Compute and store the base address of each copy of the app data */
        for (uint32_t copy_num = 0, bytes_traversed = 0, page_dsc_idx = 0;
             copy_num < CFG_APP_DATA_NUM_COPIES;
             ++copy_num)
        {
            /* Safety: page index always within bounds */
            assert(page_dsc_idx < flash.conf_ptr->ll.pages_total_num);       /* Added */

            if (copy_num == 0)
            {
                bytes_traversed = 0;
                flash.data_copies_base_addrs[copy_num] =
                    flash.conf_ptr->ll.page_descriptors[page_dsc_idx].base_addr;
                continue;
            }
            else
            {
                do
                {
                    assert(page_dsc_idx < flash.conf_ptr->ll.pages_total_num);
                    assert(copy_num < CFG_APP_DATA_NUM_COPIES);

                    if ((flash.conf_ptr->ll.page_descriptors[page_dsc_idx].size_bytes + bytes_traversed) >=
                        flash.conf_ptr->data_descriptor.data_num_bytes)
                    {
                        /* Next page becomes the base for the next copy */
                        ++page_dsc_idx;                                      /* Safe increment */
                        assert(page_dsc_idx < flash.conf_ptr->ll.pages_total_num); /* Added */

                        flash.data_copies_base_addrs[copy_num] =
                            flash.conf_ptr->ll.page_descriptors[page_dsc_idx].base_addr;
                        bytes_traversed = 0;
                        break;
                    }
                    else
                    {
                        bytes_traversed += flash.conf_ptr->ll.page_descriptors[page_dsc_idx].size_bytes;
                        ++page_dsc_idx;
                    }
                }
                while (page_dsc_idx < flash.conf_ptr->ll.pages_total_num);
            }
        }

        /* Initialise LL driver */
        ll_flash_status_t ll_status = ll_flash_init(&flash.conf_ptr->ll);
        assert(ll_status == ll_flash_status_ok);                             /* Added */
        if (ll_status != ll_flash_status_ok)
        {
            return flash_status_ll_init_fault;
        }

        /* Search for the current valid copy of app data */
        for (uint8_t idx = 0; idx < CFG_APP_DATA_NUM_COPIES; ++idx)
        {
            app_data_meta_t app_meta_data = {
                .crc32    = 0,
                .length   = 0,
                .validity = CFG_APP_DATA_INVALID
            };

            flash_read_copy_meta_data(idx, &app_meta_data);
            if (app_meta_data.validity == CFG_APP_DATA_VALID)
            {
                if (flash_load_app_data_and_check_crc(idx, &app_meta_data))
                {
                    flash.conf_ptr->data_descriptor._app_data_meta = app_meta_data;
                    flash.app_data_active_copy_base_page_idx       = idx;
                    flash.has_valid_data                           = true;
                    return flash_status_ok;
                }
                else
                {
                    /* Where we might try to roll‑back app data */
                    return flash_status_data_corruption_detected;
                }
            }
        }
    }
    else
    {
        /* Initialisation should only happen once */
        assert(0);
    }

    return flash_status_no_valid_data_found;
}

/********************************/
/*          flash_write         */
/********************************/
flash_status_t flash_write(void)
{
    flash_status_t status = flash_status_ok;

    /*--------- Sanity checks before we touch flash ---------*/
    assert(flash.initialized);                                               /* Added */
    assert(flash.conf_ptr != NULL);
    assert(flash.conf_ptr->data_descriptor.app_data != NULL);
    assert(flash.conf_ptr->data_descriptor.data_num_bytes > 0);
    assert(flash.conf_ptr->num_app_data_copies == CFG_APP_DATA_NUM_COPIES);  /* Added */

    if (!flash.initialized)
    {
        /* Should never get here due to assert above */
        return flash_status_uninitialized;
    }

    uint32_t num_bytes_to_be_erased;
    uint32_t new_copy_base_page_idx;
    app_data_meta_t new_app_meta_data = { .crc32 = 0, .length = 0, .validity = 0 };

    /* Compute base page idx of the next app_data copy region to be used */
    if ((flash.app_data_active_copy_base_page_idx + 1) < flash.conf_ptr->num_app_data_copies)
    {
        new_copy_base_page_idx = flash.app_data_active_copy_base_page_idx + 1;
    }
    else
    {
        new_copy_base_page_idx = 0;
    }

    /* Consistency between copy index and physical pages */
    assert(new_copy_base_page_idx < flash.conf_ptr->num_app_data_copies);    /* Added */

    /* Erase pages spanned by next app_data copy region */
    num_bytes_to_be_erased = flash.conf_ptr->data_descriptor.data_num_bytes + sizeof(app_data_meta_t);

    for (uint32_t page_idx = new_copy_base_page_idx;
         page_idx < flash.conf_ptr->ll.pages_total_num;
         ++page_idx)
    {
        assert(page_idx < flash.conf_ptr->ll.pages_total_num);

        if (ll_flash_page_erase(page_idx) != ll_flash_status_ok)
        {
            return flash_status_ll_erase_fault;
        }

        if (num_bytes_to_be_erased <= flash.conf_ptr->ll.page_descriptors[page_idx].size_bytes)
        {
            break;
        }
        else
        {
            num_bytes_to_be_erased -= flash.conf_ptr->ll.page_descriptors[page_idx].size_bytes;
        }
    }

    new_app_meta_data.validity = CFG_APP_DATA_VALID_CLEAR;
    new_app_meta_data.length   = flash.conf_ptr->data_descriptor.data_num_bytes;
    new_app_meta_data.crc32    =
        crc32(flash.conf_ptr->data_descriptor.app_data,
              flash.conf_ptr->data_descriptor.data_num_bytes);

    uint32_t addr = flash.data_copies_base_addrs[new_copy_base_page_idx];
    ll_flash_status_t ll_status =
        ll_flash_write(addr + sizeof(app_data_meta_t),
                       flash.conf_ptr->data_descriptor.app_data,
                       flash.conf_ptr->data_descriptor.data_num_bytes);

    if (ll_status == ll_flash_status_ok)
    {
        ll_status = ll_flash_write(addr, (uint8_t*)&new_app_meta_data, sizeof(app_data_meta_t));
        if (ll_status == ll_flash_status_ok)
        {
            /* Read back the meta data of newly written app_data and eval CRC32 */
            if (flash_load_app_data_and_check_crc(new_copy_base_page_idx, &new_app_meta_data))
            {
                /* Invalidate previous app_data copy */
                ll_flash_write(
                    flash.conf_ptr->ll.page_descriptors[flash.app_data_active_copy_base_page_idx].base_addr,
                    (uint8_t*)&(uint32_t){CFG_APP_DATA_INVALID},
                    sizeof(uint32_t));

                /* Validate new app_data copy */
                ll_flash_write(
                    flash.conf_ptr->ll.page_descriptors[new_copy_base_page_idx].base_addr,
                    (uint8_t*)&(uint32_t){CFG_APP_DATA_VALID},
                    sizeof(uint32_t));

                flash.app_data_active_copy_base_page_idx = new_copy_base_page_idx;
                status = flash_status_ok;
            }
            else
            {
                status = flash_status_crc_check_failure;
            }
        }
        else
        {
            status = flash_status_ll_write_fault;                            /* Added */
        }
    }
    else
    {
        status = flash_status_ll_write_fault;                                /* Added */
    }

    return status;
}

/******************/
/**** PRIVATES ****/
/******************/

/* Returns the TOTAL size of an app_data copy region including meta data */
static uint32_t flash_app_data_bytes_inc_meta(void)
{
    assert(flash.conf_ptr != NULL);                                          /* Added */
    uint32_t total_num_bytes = flash.conf_ptr->data_descriptor.data_num_bytes + sizeof(app_data_meta_t);
    return total_num_bytes;
}

/* Reads flash and populates app_meta_data passed via pointer */
static bool flash_read_copy_meta_data(uint32_t copy_idx, app_data_meta_t* app_meta_data)
{
    assert(app_meta_data != NULL);
    assert(copy_idx < flash.conf_ptr->num_app_data_copies);

    /* Read out meta data from base of specified page */
    if (ll_flash_read(flash.data_copies_base_addrs[copy_idx],
                      (uint8_t*)app_meta_data,
                      sizeof(app_data_meta_t)) != ll_flash_status_ok)
    {
        return false;
    }
    return true;
}

/* Loads app_data_meta_t from flash, checks CRC32 consistency.
   RETURNS: true on success, else fail. TODO: Add return status granularity */
static bool flash_load_app_data_and_check_crc(uint32_t page_idx, app_data_meta_t* app_data_meta)
{
    assert(app_data_meta != NULL);
    assert(page_idx < flash.conf_ptr->ll.pages_total_num);

    uint32_t base_addr = flash.conf_ptr->ll.page_descriptors[page_idx].base_addr;
    assert(base_addr >= flash.conf_ptr->ll.page_descriptors[CFG_APP_DATA_PAGE_ZERO].base_addr);
    assert(base_addr <= flash.conf_ptr->ll.page_descriptors[flash.conf_ptr->ll.pages_total_num - 1].base_addr);

    if (ll_flash_read(base_addr + sizeof(app_data_meta_t),
                      flash.conf_ptr->data_descriptor.app_data,
                      flash.conf_ptr->data_descriptor.data_num_bytes) != ll_flash_status_ok)
    {
        return false;
    }

    if (ll_flash_read(base_addr, (uint8_t*)app_data_meta, sizeof(app_data_meta_t)) != ll_flash_status_ok)
    {
        return false;
    }

    /* Compute CRC32 for the app_data range */
    uint32_t _crc32 = crc32(flash.conf_ptr->data_descriptor.app_data,
                            flash.conf_ptr->data_descriptor.data_num_bytes);

    /* Compare crc computed from read app data against the meta copy */
    if (_crc32 == app_data_meta->crc32)
    {
        return true;
    }

    return false;
}
