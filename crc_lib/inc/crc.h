#ifndef CRC_H
#define CRC_H
#include <stdint.h>
#include <stddef.h>

extern const uint32_t __crc32_table[256];

uint32_t crc32(const void* data, size_t nbytes);

#endif
