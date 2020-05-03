#include "stdafx.h"
#include "Network/Packets/Buffer.h"

Buffer::Buffer(const unsigned int p_size)
{
    if (p_size > 0)
    {
        data = new uint8_t[p_size];
        index = 0;
        size = p_size;
    }
}

void Buffer::Init(const unsigned int p_size)
{
    if (data != nullptr)
        throw new std::exception("Buffer already initialized");
    if (p_size > 0)
    {
        data = new uint8_t[p_size];
        index = 0;
        size = p_size;
    }
}

Buffer::~Buffer()
{
    if (data != nullptr)
        delete data;
}

void Buffer::WriteLongLong(uint64_t p_value)
{
    if (index + 8 > size)
        throw std::out_of_range("Trying to write out of buffer");

    *reinterpret_cast<uint64_t*>(data + index) = htonll(p_value);
    index += 8;
}

void Buffer::WriteInteger(const uint32_t p_value)
{
    if (index + 4 > size)
        throw std::out_of_range("Trying to write out of buffer");

    *reinterpret_cast<uint32_t*>(data + index) = htonl(p_value);
    index += 4;
}

void Buffer::WriteShort(const uint16_t p_value)
{
    if (index + 2 > size)
        throw std::out_of_range("Trying to write out of buffer");

    *reinterpret_cast<uint16_t*>(data + index) = htons(p_value);
    index += 2;
}

void Buffer::WriteByte(const uint8_t p_value)
{
    if (index + 1 > size)
        throw std::out_of_range("Trying to write out of buffer");

    *reinterpret_cast<uint8_t*>(data + index) = p_value;
    index += 1;
}

void Buffer::WriteFloat(float p_value)
{
    if (index + sizeof(float) > size)
        throw std::out_of_range("Trying to write out of buffer");

    *reinterpret_cast<float*>(data + index) = static_cast<float>(htonf(p_value));
    index += sizeof(float);
}

void Buffer::WriteBuffer(unsigned char* p_buffer, unsigned int p_size)
{
    if (index + p_size > static_cast<unsigned>(size))
        throw std::out_of_range("Trying to write out of buffer");

    for(unsigned int i = 0; i < p_size; ++i)
    {
        *reinterpret_cast<unsigned char*>(data + index) = p_buffer[i];
        ++index;
    }
}

uint64_t Buffer::ReadLongLong()
{
    if (index + 8 > size)
        throw std::out_of_range("Trying to read out of buffer");

    const uint64_t res = ntohll(*reinterpret_cast<uint64_t*>(data + index));
    index += 8;
    return res;

}
uint32_t Buffer::ReadInteger()
{
    if (index + 4 > size)
        throw std::out_of_range("Trying to read out of buffer");

    const uint32_t res = ntohl(*reinterpret_cast<uint32_t*>(data + index));
    index += 4;
    return res;
}

uint16_t Buffer::ReadShort()
{
    if (index + 2 > size)
        throw std::out_of_range("Trying to read out of buffer");

    const uint16_t res = ntohs(*reinterpret_cast<uint16_t*>(data + index));
    index += 2;
    return res;
}

uint8_t Buffer::ReadByte()
{
    if (index + 1 > size)
        throw std::out_of_range("Trying to read out of buffer");

    const uint8_t res = *reinterpret_cast<uint8_t*>(data + index);
    index += 1;
    return res;
}

float Buffer::ReadFloat()
{
    if (index + sizeof(float) > size)
        throw std::out_of_range("Trying to read out of buffer");

    float res = ntohf(*reinterpret_cast<uint8_t*>(data + index));
    index += sizeof(float);
    return res;
}

void Buffer::ReadBuffer(unsigned char* o_buffer, const unsigned int p_size)
{
    if (index + p_size > static_cast<unsigned>(size))
        throw std::out_of_range("Trying to read out of buffer");

    for(unsigned int i = 0; i < p_size; ++i)
    {
        o_buffer[i] = *reinterpret_cast<unsigned char*>(data + index);
        ++index;
    }
}
