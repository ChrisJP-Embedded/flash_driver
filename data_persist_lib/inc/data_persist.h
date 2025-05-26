#ifndef DATA_PERSIST_H
#define DATA_PERSIST_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

/* 
    Copies a number of bytes from file to program array.
    INPUT: Pointer to array to be written
    INPUT: Length of data to be written to array
    RETURNS: True on success, else False
*/
bool load_state(uint8_t* state, uint32_t len);

/* 
    Writes a number of bytes from program to file.
    RETURNS: True on success, else False
*/
void save_state(void);

#endif