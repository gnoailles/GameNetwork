#include "stdafx.h"
#include "Network/Socket.h"
#include "Network/Address.h"
#include "Network/NetworkPlugin.h"

Socket::Socket()
{
    InitializeSockets();
}

Socket::~Socket()
{
    if (IsOpen())
        Close();
}

bool Socket::Open(unsigned short p_port, const Address& p_address)
{
    InitializeSockets();
    m_handle = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    if (m_handle == INVALID_SOCKET)
    {
        g_debugCallback("Open failed : INVALID_SOCKET");
        return false;
    }

    SOCKADDR_IN address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = p_address.GetAddress();
    if(p_address.GetAddress() == 0)
        address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(static_cast<unsigned short>(p_port));

    bool value = 1;
    if (setsockopt(m_handle, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char*>(&value), 1) == SOCKET_ERROR)
    {
        g_debugCallback((std::string("Socket failed to allow reuse of address! ERROR_CODE: ") + std::to_string(NetworkAPI::SocketGetLastError())).c_str());
        return false;
    }

    if (bind(m_handle, reinterpret_cast<const SOCKADDR*>(&address), sizeof(SOCKADDR_IN)) == SOCKET_ERROR)
    {
        g_debugCallback(std::string(p_address.ToString()).c_str());
        g_debugCallback((std::string("Socket failed to bind on port ") + std::to_string(p_port) + "ERROR_CODE: " + std::to_string(NetworkAPI::SocketGetLastError())).c_str());
        return false;
    }

    DWORD nonBlocking = 1;
    if (ioctlsocket(m_handle, FIONBIO, &nonBlocking) != NO_ERROR)
    {
        g_debugCallback("Socket failed to set non-blocking");
        return false;
    }

    return true;
}

bool Socket::Close()
{
    if (m_handle == INVALID_SOCKET)
        return true;

    shutdown(m_handle, SD_BOTH);

    int res = closesocket(m_handle);
    if (res == SOCKET_ERROR)
    {
        g_debugCallback(("Closing socket failed! ERROR_CODE: " + std::to_string(WSAGetLastError())).c_str());
        return false;
    }
    m_handle = INVALID_SOCKET;
    return true;
}

bool Socket::IsOpen() const
{
    return (m_handle != INVALID_SOCKET);
}

bool Socket::AllowBroadcast() const
{
    if (m_handle == INVALID_SOCKET)
    {
        g_debugCallback("Send failed : INVALID_SOCKET");
        return false;
    }

    bool value = true;
    int res = setsockopt(m_handle,SOL_SOCKET, SO_BROADCAST, reinterpret_cast<const char *>(&value), 1);
    if (res == SOCKET_ERROR)
    {
        g_debugCallback((" Enable Broadcast failed! ERROR_CODE: " + std::to_string(WSAGetLastError())).c_str());
        return false;
    }

    return true;
}

bool Socket::Send(const Address& p_destination, const unsigned char* p_data, const int p_size) const
{
    if (m_handle == INVALID_SOCKET)
    {
        g_debugCallback("Send failed : INVALID_SOCKET");
        return false;
    }

    SOCKADDR_IN address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = p_destination.GetAddress();
    address.sin_port = p_destination.GetNPort();

    const int sentBytes = sendto(m_handle, reinterpret_cast<const char*>(p_data), p_size, 0,
                                 reinterpret_cast<SOCKADDR*>(&address), sizeof(SOCKADDR_IN));
    if (sentBytes == SOCKET_ERROR)
    {
        g_debugCallback(("Send failed! ERROR_CODE: " + std::to_string(WSAGetLastError())).c_str());
        return false;
    }
    return sentBytes == p_size;
}

bool Socket::Send(const char* p_address, const short p_port, const unsigned char* p_data, int p_size) const
{
    SOCKADDR_IN address;

    inet_pton(AF_INET, p_address, &(address.sin_addr));
    address.sin_family = AF_INET;
    address.sin_port = htons(p_port);

    const int sentBytes = sendto(m_handle, reinterpret_cast<const char*>(p_data), p_size, 0,
                                 reinterpret_cast<SOCKADDR*>(&address), sizeof(SOCKADDR_IN));

    if (sentBytes == SOCKET_ERROR)
    {
        g_debugCallback(("Send failed! ERROR_CODE: " + std::to_string(WSAGetLastError())).c_str());
        return false;
    }

    return sentBytes == p_size;
}

int Socket::Receive(Address& o_sender, unsigned char* o_data, int p_size) const
{
    if (m_handle == INVALID_SOCKET)
    {
        g_debugCallback("Receive failed : INVALID_SOCKET");
        return -1;
    }

    SOCKADDR_IN from;
    int fromSize = sizeof from;

    const int bytes = recvfrom(m_handle, reinterpret_cast<char*>(o_data), p_size, 0, reinterpret_cast<SOCKADDR*>(&from),
                               &fromSize);
    if (bytes == SOCKET_ERROR)
    {
        const int error = WSAGetLastError();
        if (error != WSAEWOULDBLOCK)
            g_debugCallback(("Receive failed! ERROR_CODE: " + std::to_string(error)).c_str());
        return -1;
    }
    o_sender = { ntohl(from.sin_addr.s_addr), ntohs(from.sin_port) };
    return bytes;
}


#pragma region CExport
extern "C"
{
    Socket* Internal_SocketCreate()
    {
        return new Socket;
    }

    void Internal_SocketDestroy(Socket* p_obj)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }

        delete p_obj;
        p_obj = nullptr;
    }

    bool Internal_SocketOpen(Socket* p_obj, unsigned short p_port)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }
        return p_obj->Open(p_port);
    }

    bool Internal_SocketClose(Socket* p_obj)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }
        return p_obj->Close();
    }

    bool Internal_SocketIsOpen(Socket* p_obj)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }

        return p_obj->IsOpen();
    }

    bool Internal_SocketSend(Socket* p_obj, const Address& p_destination, const unsigned char* p_data, int p_size)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }

        return p_obj->Send(p_destination, p_data, p_size);
    }

    bool Internal_SocketSend_string(Socket* p_obj, const char* p_address, const short p_port, const unsigned char* p_data, int p_size)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }

        return p_obj->Send(p_address, p_port, p_data, p_size);
    }

    int Internal_SocketReceive(Socket* p_obj, Address& o_sender,unsigned char* o_data, int p_size)
    {
        if (!p_obj)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }

        return p_obj->Receive(o_sender, o_data, p_size);
    }
}
#pragma endregion 