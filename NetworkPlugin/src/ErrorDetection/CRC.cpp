#include "stdafx.h"
#include "Network/ErrorDetection/CRC.h"

uint_fast32_t   CRC32::table[256] = {};
bool            CRC32::isTableInit = false;