#pragma once
#include "export.h"

class Address
{
private:
    unsigned int m_address;
    unsigned short m_port;

public:

    Address();
    Address(unsigned char p_a, unsigned char p_b, unsigned char p_c, unsigned char p_d, unsigned short p_port);
    Address(unsigned int p_address, unsigned short p_port);
    Address(const char* p_address, unsigned short p_port);

    Address(const Address& p_other) = default;
    Address(Address&& p_other) noexcept;

    Address& operator=(const Address& p_other);
    Address& operator=(Address&& p_other) noexcept;

    ~Address() = default;

    inline unsigned int	  GetAddress() const;
    inline unsigned short GetNPort() const;
    inline unsigned short GetPort() const;

    inline unsigned char  GetA() const;
    inline unsigned char  GetB() const;
    inline unsigned char  GetC() const;
    inline unsigned char  GetD() const;

    std::string           ToString() const;
    bool operator==(const Address& p_other) const;
    bool operator!=(const Address& p_other) const;
};

#pragma region CExport
extern "C"
{
    NETWORK_PLUGIN_API Address* Internal_AddressCreate_uchar(unsigned char p_a, unsigned char p_b, unsigned char p_c, unsigned char p_d, unsigned short p_port);
    NETWORK_PLUGIN_API Address* Internal_AddressCreate_uint(unsigned int p_address, unsigned short p_port);
    NETWORK_PLUGIN_API Address* Internal_AddressCreate_char(const char* p_address, unsigned short p_port);

    NETWORK_PLUGIN_API Address* Internal_AddressCreateCopy(const Address& p_other);

    NETWORK_PLUGIN_API void Internal_AddressDestroy(Address* p_obj);

    NETWORK_PLUGIN_API inline unsigned int   Internal_AddressGetAddress(const Address* p_obj);
    NETWORK_PLUGIN_API inline unsigned short Internal_AddressGetNPort(const Address* p_obj);
    NETWORK_PLUGIN_API inline unsigned short Internal_AddressGetPort(const Address* p_obj);


    NETWORK_PLUGIN_API inline unsigned char  Internal_AddressGetA(const Address* p_obj);
    NETWORK_PLUGIN_API inline unsigned char  Internal_AddressGetB(const Address* p_obj);
    NETWORK_PLUGIN_API inline unsigned char  Internal_AddressGetC(const Address* p_obj);
    NETWORK_PLUGIN_API inline unsigned char  Internal_AddressGetD(const Address* p_obj);

    NETWORK_PLUGIN_API const char* Internal_AddressToString(const Address* p_obj);
}
#pragma endregion 