#pragma once

struct Buffer
{
    uint8_t* data = nullptr;    // pointer to buffer data
    int size = 0;               // size of buffer data (bytes)
    int index = 0;     // index of next byte to be read/written

    Buffer() = default;
    explicit Buffer(unsigned int p_size);
    void Init(unsigned int p_size);
    ~Buffer();

    void        WriteLongLong(uint64_t p_value);
    void        WriteInteger(uint32_t p_value);
    void        WriteShort(uint16_t p_value);
    void        WriteByte(uint8_t  p_value);
    void        WriteFloat(float p_value);
    void        WriteBuffer(unsigned char* p_buffer, unsigned int p_size);

    uint64_t    ReadLongLong();
    uint32_t    ReadInteger();
    uint16_t    ReadShort();
    uint8_t     ReadByte();
    float       ReadFloat();
    void        ReadBuffer(unsigned char* o_buffer, unsigned int p_size);

};