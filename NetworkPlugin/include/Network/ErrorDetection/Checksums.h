#pragma once
#include <cstdint>
uint32_t Adler32(const uint16_t* p_data, const unsigned int p_size);
uint32_t Fletcher32(const uint16_t* p_data, const unsigned int p_size);
uint32_t Fletcher32_improved(const uint16_t* p_data, unsigned int p_size);
