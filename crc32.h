/*
* Code Adapted from http://www.barrgroup.com/Embedded-Systems/How-To/CRC-Calculation-C-Code
* Code generates the same CRC as "crc32()" function on target hardware
*/
#include <stdint.h>

static const uint32_t POLYNOMIAL = 0x04C11DB7;          // IEEE 802.3 Polynomial
static const uint32_t INITIAL_REMAINDER = 0xFFFFFFFF;
static const uint32_t FINAL_XOR_VALUE = 0xFFFFFFFF;    
static const uint32_t WIDTH = 32;                       // (8 * sizeof(uint32_t));
static const uint32_t TOPBIT = (1 << 31);              // (WIDTH - 1)

static uint32_t reflect(uint32_t data, uint8_t nBits)
{
	uint32_t reflection = 0;
	for (uint8_t bit = 0; bit < nBits; ++bit)
	{
		if (data & 0x01)
			reflection |= (1 << ((nBits - 1) - bit));

		data = (data >> 1);
	}
	return reflection;
}

uint32_t crc32_gen(const uint8_t* const message, uint32_t nBytes)
{
    uint32_t remainder = INITIAL_REMAINDER;
    for(uint32_t byte = 0; byte < nBytes; ++byte)
    {
        remainder ^= (reflect(message[byte], 8) << (WIDTH - 8));
        for (uint8_t bit = 8; bit > 0; --bit)
        {
            if (remainder & TOPBIT)
                remainder = (remainder << 1) ^ POLYNOMIAL;
            else
                remainder = (remainder << 1);
        }
    }
    return (reflect(remainder, WIDTH) ^ FINAL_XOR_VALUE);
}
