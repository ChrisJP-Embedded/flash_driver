#include "file_io.h"

/* 
    This module ONLY exists to provide faux 
    flash persistence via file save / load.
*/

uint8_t* state_ptr;
uint32_t state_len;

bool load_state(uint8_t* state, uint32_t num_bytes)
{
    FILE *f = fopen("nv_state", "rb");

    // Cache values
    state_ptr = state;
    state_len = num_bytes;

    if (f == NULL) 
    {
        printf("Failed to load array\n");
        return false;
    }

    if(fread(state, sizeof(uint8_t), num_bytes, f) != num_bytes)
    {
        printf("ERROR: NV load failed\n");
        return false;
    }

    fclose(f);
    return true;
}

void save_state(void)
{
    FILE *f = fopen("nv_state", "wb");

    if (f == NULL) 
    {
        printf("ERROR: Could not open file to dump NV\n");
        return;
    }

    if(fwrite(state_ptr, sizeof(uint8_t), state_len, f) != state_len)
    {
        printf("ERROR: NV dump bytes written does not match requested\n");
    }
    fflush(f);
    fclose(f);
}