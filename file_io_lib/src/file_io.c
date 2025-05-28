#include "file_io.h"

/* 
    This module ONLY exists to provide faux 
    flash persistence via file save / load.
*/

bool load_state(uint8_t *state, uint32_t num_bytes)
{
    if (state == NULL || num_bytes == 0)
    {
        printf("file_io:load_state: bad arguments, failed\n");
        return false;
    }

    FILE *f = fopen("nv_state", "rb");

    if (f == NULL)
    {
        printf("file_io:load_state: fopen failed\n");
        return false;
    }

    if (fread(state, 1, num_bytes, f) != (size_t)num_bytes)
    {
        printf("file_io:load_state: fread failed\n");
        fclose(f);
        return false;
    }

    if (fclose(f) != 0)
    {
        printf("file_io:load_state: fclose failed\n");
        return false;
    }

    return true;  // TODO: switch to a status enum
}

bool save_state(uint8_t *state, uint32_t num_bytes)
{
    if (state == NULL || num_bytes == 0)
    {
        printf("file_io:save_state: bad arguments, failed\n");
        return false;
    }

    FILE *f = fopen("nv_state", "wb");

    if (f == NULL)
    {
        printf("file_io:save_state: fopen failed\n");
        return false;
    }

    // Write state to file
    if (fwrite(state, 1, num_bytes, f) != (size_t)num_bytes)
    {
        printf("file_io:save_state: fwrite failed\n");
        fclose(f);
        return false;
    }

    // force flush
    if (fflush(f) != 0)
    {
        printf("file_io:save_state: fflush failed\n");
        fclose(f);
        return false;
    }

    if (fclose(f) != 0)
    {
        printf("file_io:save_state: fclose failed\n");
        return false;
    }

    return true;  // TODO: switch to a status enum
}
