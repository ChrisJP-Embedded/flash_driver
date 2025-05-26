#include <string.h>
#include <assert.h>
#include "ll_flash.h"
#include "file_io.h"

#define FLASH_SIZE 1024UL*128UL*4UL

ll_flash_config_t* ll_flash_ptr = NULL;

uint8_t mem[FLASH_SIZE]; // DEV/TEST ARTIFACT

ll_flash_status_t ll_flash_init(ll_flash_config_t* _ll_flash_ptr)
{
    assert(_ll_flash_ptr != NULL);
    ll_flash_ptr = _ll_flash_ptr;
    if(load_state(mem, FLASH_SIZE) == false)
    {
        printf("stub nv load failed, flash in earsed state\n");
    }
    return ll_flash_status_ok;
}

ll_flash_status_t ll_flash_read(uint32_t addr, uint8_t* data, uint32_t num_bytes)
{
    // Offset the addr to zero index for stub fake mem array indexing
    assert(ll_flash_ptr != NULL);
    addr -= ll_flash_ptr->page_descriptors[0].base_addr;
    assert(addr < FLASH_SIZE);
    memcpy(data, mem + addr, num_bytes);
    return ll_flash_status_ok;
}

ll_flash_status_t ll_flash_write(uint32_t addr, uint8_t* data, uint32_t num_bytes)
{
    // Offset the addr to zero index for stub fake mem array indexing
    assert(ll_flash_ptr != NULL);
    addr -= ll_flash_ptr->page_descriptors[0].base_addr;
    assert(addr < FLASH_SIZE);
    memcpy(mem + addr, data, num_bytes);
    save_state();
    return ll_flash_status_ok;
}

ll_flash_status_t ll_flash_page_erase(uint8_t page_idx)
{
    // Offset the addr to zero index for stub fake mem array indexing
    uint32_t addr = ll_flash_ptr->page_descriptors[page_idx].base_addr;
    addr -= ll_flash_ptr->page_descriptors[0].base_addr;
    assert(addr < FLASH_SIZE);
    for(;addr < ll_flash_ptr->page_descriptors[page_idx].size_bytes; ++addr)
    {
        mem[addr] = 0xFF;
    }
    save_state();
    return ll_flash_status_ok;
}