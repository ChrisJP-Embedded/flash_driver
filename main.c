#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "flash.h"
#include "flash_conf.h"

/*
A test simple harness for simple flash driver module.

Uses python to drive a variable length series of actions
via a set of opcodes passed to main arg vector. Python 
code in python/flash_test.py.

The low-level flash driver (device-specific level) is a stub
which saves and loads an array to file, providing faux flash.

Python can analyze the faux flash file and could (not implemented)
also update the app_data and size via file IO in the opcode dispatcher.
*/

static flash_status_t _flash_init(flash_config_t*);
static flash_status_t _flash_write(void);

// For testing only - some super important
// app data imitation for testing flash module
#define TEST_DATA_LEN NUM_KB_TO_NUM_BYTE(134)
uint8_t test_data_a[TEST_DATA_LEN];

// Flash app-level configuration
flash_config_t flash_config = 
{
    // TODO: Update to include the following:
    // - max_app_data_size (sets aside extra data for expansion!)
    // - an array of data descriptors for app data?
    // - add separate app_data_base_ptr and meta_app_data_base_ptr
	.num_app_data_copies = CFG_APP_DATA_NUM_COPIES,
	.data_descriptor = {
            //TODO: Think about a nicer way to describe num bytes of
            //actual app_data and the app_data_meta (which is prefixed)
			.data_num_bytes = sizeof(test_data_a),
			.app_data = (uint8_t*)&test_data_a,
	}, 

    // The ll (low-level) configuration.

    // If the flash has keys we can have them
    // on hand ready to iterate through reg writes
    .ll.num_flash_keys = CFG_NUM_FLASH_KEYS,
    .ll.flash_keys = (const uint32_t[CFG_NUM_FLASH_KEYS]){CFG_FLASH_KEY1, CFG_FLASH_KEY2},

    // No sector support yet - but would likely just be
    // similar to page descriptors (below) and used to perform
    // erases spanning multiple pages. Not a must have, as is mass erase.

    // By having an array of page descriptors we have all the 
    // page base addresses and respective sizes on hand at runtime.
    // Also solves problem of system with various page sizes.
    .ll.pages_total_num = CFG_NUM_PAGES,
    .ll.page_descriptors = (const page_dsc_t[CFG_NUM_PAGES]){
        {
            .base_addr = CFG_PAGE1_BASE_ADDR,
            .size_bytes = CFG_PAGE1_NUM_BYTES,
        },
        {
            .base_addr = CFG_PAGE2_BASE_ADDR,
            .size_bytes = CFG_PAGE2_NUM_BYTES,
        },
        {
            .base_addr = CFG_PAGE3_BASE_ADDR,
            .size_bytes = CFG_PAGE3_NUM_BYTES, 
        },
        {
            .base_addr = CFG_PAGE4_BASE_ADDR,
            .size_bytes = CFG_PAGE4_NUM_BYTES, 
        },
    },
};

// Init some test data to work on
static void init_test_data(void)
{
    // Fill with sequential non-zero values 
    for(uint32_t idx = 0; idx < TEST_DATA_LEN; ++idx)
    {
        test_data_a[idx] = (idx + 1);
    }
}

int main(int argc, char *argv[])
{
    char** tests_opcode_ptr = NULL;
    flash_status_t init_status;
    int tests_num_opcodes;

    enum // scoped symbols
    {
        NUM_TEST_OPCODES = 1,
        TEST_OPCODE_0 = 2,
        VALID_TEST_SEQUENCE = 3,
    };

    if(argc >= VALID_TEST_SEQUENCE)
    {
        if((argv[NUM_TEST_OPCODES] != NULL) && (argv[TEST_OPCODE_0] != NULL))
        {

            tests_num_opcodes = atoi(argv[NUM_TEST_OPCODES]);
            tests_opcode_ptr = argv + TEST_OPCODE_0;

            printf("\nmain: num test opcodes: %d\n", tests_num_opcodes);

            enum // scoped symbols
            {
                INIT,
                WRITE,
                UPDATE_APP_DATA,
                INIT_APP_DATA,
            };

            // By using an array of opcodes we can sequence actions
            // which drive the flash api and manipulate app data
            while ((tests_num_opcodes > 0) && (*tests_opcode_ptr != NULL))
            {
                switch(atoi(*tests_opcode_ptr++))
                {
                    case INIT:
                        printf("main: attempting flash initialization\n");
                        _flash_init(&flash_config);
                        break;

                    case WRITE:
                        printf("main: attempting flash write op\n");
                        _flash_write();
                        break;

                    case UPDATE_APP_DATA:
                        // Update data not implemented, would use
                        // file IO to read file produced by python
                        // and hence update the app data.
                        break;

                    case INIT_APP_DATA:
                        printf("main: initializing test data\n");
                        init_test_data();
                        break;

                    default:
                        printf("main: unrecognised opcode\n");
                        break; 
                }

                --tests_num_opcodes;
            }
        }
    }
    else
    {
        // manual bits when main not being
        // driven by python test dispatcher
        init_test_data();
        _flash_init(&flash_config);
        _flash_write();
    }

    return 0;
}

/*
    Encapsulate flash initialization
    status console output
*/
static flash_status_t _flash_init(flash_config_t* flash_config)
{
    // Initialize flash driver with our apps configuration
    flash_status_t status = flash_init(flash_config);

    switch(status)
    {
        case flash_status_ok:
            printf("main:init: flash good!\n");
            break;

        case flash_status_total_size_exceeded:
            printf("main:init: requested app data layout exceeds available flash\n");
            break;

        case flash_status_data_corruption_detected:
            printf("main:init: data corruption detected\n");
            break;

        case flash_status_no_valid_data_found:
            printf("main:init: no valid data found\n");
            break;

        case flash_status_crc_check_failure:
            printf("main:init: crc check failure - corrupted data");
            break;

        case flash_status_ll_init_fault:
            printf("main:init: ll stub reported issue - OK if nv_data didn't exist on first run\n");
            break;

        default:
            printf("main:init: unexpected state!\n");
            assert(0);
            break;
    }

    return status;
}

/*
    Encapsulate flash write
    status console output
*/
static flash_status_t _flash_write(void)
{
    flash_status_t status = flash_write();

    switch(status)
    {
        case flash_status_ok:
            printf("main: write good!\n");
            break;

        default:
            printf("main: write fail: %d\n", status);
            break;
    }

    return status;
}
