#pragma once
#include "export.h"
#include "Address.h"

class Address;

class Socket
{
private:
    SOCKET m_handle{ INVALID_SOCKET };

public:
    Socket();
    ~Socket();

    bool Open(unsigned short p_port, const Address& p_address = {static_cast<unsigned>(INADDR_ANY), 0});
    bool Close();

    bool IsOpen() const;

    bool AllowBroadcast() const;

    bool Send(const Address& p_destination, const unsigned char* p_data, int p_size) const;
    bool Send(const char* p_address, const short p_port, const unsigned char* p_data, int p_size) const;
    int Receive(Address & o_sender, unsigned char* o_data, int p_size) const;
};

#pragma region CExport
extern "C"
{
    NETWORK_PLUGIN_API Socket*  Internal_SocketCreate();
    NETWORK_PLUGIN_API void     Internal_SocketDestroy(Socket* p_obj);

    NETWORK_PLUGIN_API bool     Internal_SocketOpen(Socket* p_obj, unsigned short p_port);
    NETWORK_PLUGIN_API bool     Internal_SocketClose(Socket* p_obj);

    NETWORK_PLUGIN_API bool     Internal_SocketIsOpen(Socket* p_obj);

    NETWORK_PLUGIN_API bool     Internal_SocketSend(Socket* p_obj, const Address& p_destination, const unsigned char* p_data, int p_size);
    NETWORK_PLUGIN_API bool     Internal_SocketSend_string(Socket* p_obj, const char* p_address, const short p_port, const unsigned char* p_data, int p_size);
    NETWORK_PLUGIN_API int      Internal_SocketReceive(Socket* p_obj, Address& o_sender, unsigned char* o_data, int p_size);
}
#pragma endregion 