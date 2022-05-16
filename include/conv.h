#include <stdlib.h>

#ifndef _conv_H
#define _conv_H

uint16_t conv_b2u16(uint8_t* buf);
uint32_t conv_b2u32(uint8_t* buf);
uint64_t conv_b2u64(uint8_t* buf);

void conv_u162b(uint8_t* buf, uint64_t val);
void conv_u642b(uint8_t* buf, uint64_t val);

#endif