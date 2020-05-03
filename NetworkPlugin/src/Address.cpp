#include "stdafx.h"
#include "Network/Address.h"
#include "Network/NetworkPlugin.h"

Address::Address() : m_address(0), m_port(0)
{
}

Address::Address(const unsigned char p_a,
                 const unsigned char p_b,
                 const unsigned char p_c,
                 const unsigned char p_d,
                 const unsigned short p_port) :
    m_address{ static_cast<unsigned int>((p_a << 24) | (p_b << 16) | (p_c << 8) | p_d) },
    m_port(p_port)
{}

Address::Address(const unsigned int p_address, const unsigned short p_port) : m_address(p_address), m_port(p_port)
{
}

Address::Address(const char* p_address, unsigned short p_port) : m_address(), m_port(p_port)
{
    if (inet_pton(AF_INET, p_address, &m_address) == 0)
        g_debugCallback("Invalid String");
    m_address = ntohl(m_address);
}

Address::Address(Address&& p_other) noexcept : m_address(p_other.m_address),
m_port(p_other.m_port)
{
}

Address& Address::operator=(const Address& p_other)
{
    if (this == &p_other)
        return *this;
    m_address = p_other.m_address;
    m_port = p_other.m_port;
    return *this;
}

Address& Address::operator=(Address&& p_other) noexcept
{
    if (this == &p_other)
        return *this;
    m_address = p_other.m_address;
    m_port = p_other.m_port;
    return *this;
}

unsigned Address::GetAddress() const
{
    return htonl(m_address);
}

unsigned short Address::GetNPort() const
{
    return htons(m_port);
}

unsigned short Address::GetPort() const
{
    return m_port;
}

unsigned char Address::GetA() const
{
    return m_address >> 24;
}

unsigned char Address::GetB() const
{
    return (m_address >> 16) & 0xFF;
}

unsigned char Address::GetC() const
{
    return (m_address >> 8) & 0xFF;
}

unsigned char Address::GetD() const
{
    return m_address & 0xFF;
}

std::string Address::ToString() const
{
    return (std::to_string(GetA()) + "." +
            std::to_string(GetB()) + "." +
            std::to_string(GetC()) + "." +
            std::to_string(GetD()) + ":" +
            std::to_string(m_port));
}

bool Address::operator==(const Address& p_other) const
{
    return (m_address == p_other.m_address && m_port == p_other.m_port);
}
bool Address::operator!=(const Address& p_other) const
{
    return (m_address != p_other.m_address || m_port != p_other.m_port);
}


#pragma region CExport
extern "C"
{
    Address* Internal_AddressCreate_uchar(unsigned char p_a, unsigned char p_b, unsigned char p_c, unsigned char p_d,
                                          unsigned short p_port)
    {
        return new Address(p_a, p_b, p_c, p_d, p_port);
    }

    Address* Internal_AddressCreate_uint(unsigned int p_address, unsigned short p_port)
    {
        return new Address(p_address, p_port);
    }

    Address* Internal_AddressCreate_char(const char* p_address, unsigned short p_port)
    {
        return new Address(p_address, p_port);
    }

    Address* Internal_AddressCreateCopy(const Address& p_other)
    {
        return new Address(p_other);
    }

    void Internal_AddressDestroy(Address* p_obj)
    {
        if (!p_obj)
            return;
        delete p_obj;
        p_obj = nullptr;
    }

    unsigned Internal_AddressGetAddress(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetAddress();
    }

    unsigned short Internal_AddressGetNPort(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetNPort();
    }

    unsigned short Internal_AddressGetPort(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetPort();
    }

    unsigned char Internal_AddressGetA(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetA();
    }

    unsigned char Internal_AddressGetB(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetB();
    }

    unsigned char Internal_AddressGetC(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetC();
    }

    unsigned char Internal_AddressGetD(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->GetD();
    }

    const char* Internal_AddressToString(const Address* p_obj)
    {
        if (!p_obj)
            return NULL;
        return p_obj->ToString().c_str();
    }
}
#pragma endregion 
