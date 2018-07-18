#pragma once
#include <cstdint>

#define SHA256_BLOCK_SIZE 32

typedef unsigned char BYTE;             // 8-bit byte
typedef struct {
	BYTE data[64];
	uint32_t datalen;
	unsigned long long bitlen;
	uint32_t state[8];
} SHA256_CTX;



