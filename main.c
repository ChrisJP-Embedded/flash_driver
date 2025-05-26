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

Python can analyse the faux flash file and could (not implemented)
also update the app_data and size via file IO in the opcode dispatcher.
*/

// For testing only - saves/loads RAM
// flash imitation array to/from file
#include "file_io.h"

static flash_status_t _flash_init(void);
static flash_status_t _flash_write(void);

// For testing only - some super important
// app data imitation for testing flash module
#define TEST_DATA_LEN NUM_KB_TO_NUM_BTYE(134)
uint8_t test_data_a[TEST_DATA_LEN];

// Flash app-level configuartion
flash_config_t flash_config = 
{
    // TODO: Update to include the following:
    // - max_app_data_size (sets aside extra data for expansion!)
    // - an array of data descriptors for app data?
    // - add seperate data_ptr and meta_data_ptr
	.num_app_data_copies = CFG_APP_DATA_NUM_COPIES,
	.data_descriptor = {
			.data_num_bytes = sizeof(test_data_a),
			.data_ptr = (uint8_t*)&test_data_a,
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
        VALID_TEST_SEQUENCE = 3,
        NUM_TEST_OPCODES = 1,
        TEST_OPCODE_0 = 2,
    };

    if(argc >= VALID_TEST_SEQUENCE)
    {
        if((argv[NUM_TEST_OPCODES] != NULL) && (argv[TEST_OPCODE_0] != NULL))
        {

            tests_num_opcodes = atoi(argv[NUM_TEST_OPCODES]);
            tests_opcode_ptr = argv + TEST_OPCODE_0;

            printf("num test opcodes: %d\n", tests_num_opcodes);

            enum // scoped symbols
            {
                INIT,
                WRITE,
                UPDATE_APP_DATA,
                INIT_APP_DATA,
            };

            // By using an array of opcodes we can sequence actions
            // which drive the flash api and manipulate app data
            while (tests_num_opcodes > 0 || tests_opcode_ptr == NULL)
            {
                switch(atoi(*tests_opcode_ptr++))
                {
                    case INIT:
                        printf("Initialize flash\n");
                        _flash_init();
                        break;

                    case WRITE:
                        printf("Flash write op\n");
                        _flash_write();
                        break;

                    case UPDATE_APP_DATA:
                        // Update data not implemented, would use
                        // file IO to read file produced by python
                        // and hence update the app data.
                        break;

                    case INIT_APP_DATA:
                        init_test_data();
                        break;

                    default:
                        printf("assert");
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
    }

    return 0;
}

/*
    Encapsulate flash initialization
    and handling of return values
*/
static flash_status_t _flash_init(void)
{
    // Initialize flash driver with our apps configuration
    flash_status_t status = flash_init(&flash_config);

    switch(status)
    {
        case flash_status_ok:
            printf("flash good!\n");
            break;

        case flash_status_total_size_exceeded:
            printf("requested app data layout exceeds available flash\n");
            break;

        case flash_status_data_corruption_detected:
            printf("data corruption detected\n");
            break;

        case flash_status_no_valid_data_found:
            printf("no valid data found\n");
            break;

        default:
            printf("unexpected state!\n");
            assert(0);
            break;
    }

    return status;
}

static flash_status_t _flash_write(void)
{
    flash_status_t status = flash_write();

    switch(status)
    {
        case flash_status_ok:
            printf("write good!\n");
            break;

        default:
            printf("write fail: %d\n", status);
            break;
    }

    return status;
}