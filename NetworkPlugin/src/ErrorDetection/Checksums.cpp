#include "stdafx.h"
#include "Network/ErrorDetection/Checksums.h"


uint32_t Adler32(const uint16_t* p_data, const unsigned int p_size)
{
	uint32_t sumA = 1;
	uint32_t sumB = 0;
	unsigned int j = p_size;
	for(unsigned int i = 0; i < p_size; ++i, --j) 
	{
		sumA = (sumA + p_data[i])	% 65521;
		sumB = (sumB + sumA)		% 65521;
	}

	return (sumB << 16) | sumA;
}

uint32_t Fletcher32(const uint16_t* p_data, const unsigned int p_size)
{
	uint32_t sumA = 0;
	uint32_t sumB = 0;
	for(unsigned int i = 0; i < p_size; ++i )
	{
		sumA = (sumA + p_data[i])	% 65535;
		sumB = (sumB + sumA)		% 65535;
	}
	return (sumB << 16) | sumA;
}


// Improved Fletcher32 by delaying modulo until necessary (every 360 sums)
uint32_t Fletcher32_improved(const uint16_t* p_data, unsigned int p_size)
{
	uint32_t sumA, sumB;
	unsigned int i;

	for (sumA = sumB = 0; p_size >= 360; p_size -= 360) 
	{
	    for (i = 0; i < 360; ++i) 
		{
	            sumA = sumA + *p_data++;
	            sumB = sumB + sumA;
	    }
	    sumA = sumA % 65535;
	    sumB = sumB % 65535;
	}
	for (i = 0; i < p_size; ++i) 
	{
	        sumA = sumA + *p_data++;
	        sumB = sumB + sumA;
	}
	sumA = sumA % 65535;
	sumB = sumB % 65535;
	return (sumB << 16 | sumA);
}
