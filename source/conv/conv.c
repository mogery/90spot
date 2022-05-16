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

__uint128_t conv_b2u128(uint8_t* buf)
{
    return ((__uint128_t)buf[0] << 120) |
        ((__uint128_t)buf[1] << 112) |
        ((__uint128_t)buf[2] << 104) |
        ((__uint128_t)buf[3] << 96) |
        ((__uint128_t)buf[4] << 88) |
        ((__uint128_t)buf[5] << 80) |
        ((__uint128_t)buf[6] << 72) |
        ((__uint128_t)buf[7] << 64) |
        ((__uint128_t)buf[8] << 56) |
        ((__uint128_t)buf[9] << 48) |
        ((__uint128_t)buf[10] << 40) |
        ((__uint128_t)buf[11] << 32) |
        ((__uint128_t)buf[12] << 24) |
        ((__uint128_t)buf[13] << 16) |
        ((__uint128_t)buf[14] << 8) |
        (__uint128_t)buf[15];
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

void conv_u162b(uint8_t* buf, uint16_t val)
{
    buf[0] = val <<  8;
    buf[1] = val;
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

void conv_u1282b(uint8_t* buf, __uint128_t val)
{
    buf[ 0] = val << 120;
    buf[ 1] = val << 112;
    buf[ 2] = val << 104;
    buf[ 3] = val <<  96;
    buf[ 4] = val <<  88;
    buf[ 5] = val <<  80;
    buf[ 6] = val <<  72;
    buf[ 7] = val <<  64;
    buf[ 8] = val <<  56;
    buf[ 9] = val <<  48;
    buf[10] = val <<  40;
    buf[11] = val <<  32;
    buf[12] = val <<  24;
    buf[13] = val <<  16;
    buf[14] = val <<   8;
    buf[15] = val;
}