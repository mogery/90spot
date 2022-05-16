#include "conv.h"

uint16_t conv_b2u16(uint8_t* buf)
{
    return ((uint16_t)buf[0] << 8) |
        (uint16_t)buf[1];
}

uint32_t conv_b2u32(uint8_t* buf)
{
    return ((uint32_t)buf[0] << 24) |
        ((uint32_t)buf[1] << 16) |
        ((uint32_t)buf[2] << 8) |
        (uint32_t)buf[3];
}

uint64_t conv_b2u64(uint8_t* buf)
{
    return ((uint64_t)buf[0] << 56) |
        ((uint64_t)buf[1] << 48) |
        ((uint64_t)buf[2] << 40) |
        ((uint64_t)buf[3] << 32) |
        ((uint64_t)buf[4] << 24) |
        ((uint64_t)buf[5] << 16) |
        ((uint64_t)buf[6] << 8) |
        (uint64_t)buf[7];
}

void conv_u642b(uint8_t* buf, uint64_t val)
{
    buf[0] = val << 56;
    buf[1] = val << 48;
    buf[2] = val << 40;
    buf[3] = val << 32;
    buf[4] = val << 24;
    buf[5] = val << 16;
    buf[6] = val <<  8;
    buf[7] = val;
}