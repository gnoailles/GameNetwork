#pragma once
#include <array>
#include <numeric>

class CRC32
{
    static const    uint_fast64_t   POLYNOMIAL          = 0x04C11DB7;
    static const    uint_fast32_t   POLYNOMIAL_WIDTH    = 8 * sizeof uint_fast32_t;
    static const    uint_fast32_t   TOP_BIT             = (1 << (POLYNOMIAL_WIDTH - 1));
    static const    uint_fast32_t   INITIAL_REMAINDER   = 0xFFFFFFFF;
    static const    uint_fast32_t   FINAL_XOR_VALUE     = 0xFFFFFFFF;

    static uint_fast32_t table[256];
    static bool	isTableInit;

    static uint_fast32_t ReverseBits(uint_fast32_t p_number, unsigned int p_nBits)
    {
        uint_fast32_t reverse = p_number;

        while (p_number)
        {
            reverse <<= 1;
            reverse |= p_number & 1;
            p_number >>= 1;
            --p_nBits;
        }

        reverse <<= p_nBits;
        return reverse;
    }

public:
    static void	InitTable()
    {
        if (isTableInit)
            return;
        for (int dividend = 0; dividend < 256; ++dividend)
        {
            //Add (Width - 1) 0s
            uint_fast32_t remainder = dividend << (POLYNOMIAL_WIDTH - 8);
            for (unsigned char bit = 8; bit > 0; --bit)
            {
                if (remainder & TOP_BIT)
                    remainder = (remainder << 1) ^ POLYNOMIAL;
                else
                    remainder = (remainder << 1);
            }
            table[dividend] = remainder;
        }
        isTableInit = true;
    }

    static uint_fast32_t GetCRCTableBased(unsigned char const p_data[], unsigned int p_nBytes)
    {
        uint_fast32_t	remainder = INITIAL_REMAINDER;

        for (unsigned int byte = 0; byte < p_nBytes; ++byte)
        {
            unsigned char data = ReverseBits(p_data[byte], 8) ^ (remainder >> (POLYNOMIAL_WIDTH - 8));
            remainder = table[data] ^ (remainder << 8);
        }

        return (static_cast<uint_fast32_t>(ReverseBits(remainder, POLYNOMIAL_WIDTH)) ^ FINAL_XOR_VALUE);
    }

    static uint_fast32_t GetCRC(unsigned char const message[], int nBytes)
    {
        uint_fast32_t	remainder = INITIAL_REMAINDER;

        for (int byte = 0; byte < nBytes; ++byte)
        {
            remainder ^= (ReverseBits(message[byte], 8) << (POLYNOMIAL_WIDTH - 8));

            for (unsigned char bit = 8; bit > 0; --bit)
            {
                if (remainder & TOP_BIT)
                    remainder = (remainder << 1) ^ POLYNOMIAL;
                else
                    remainder = (remainder << 1);
            }
        }
        return (static_cast<uint_fast32_t>(ReverseBits(remainder, POLYNOMIAL_WIDTH)) ^ FINAL_XOR_VALUE);
    }
};