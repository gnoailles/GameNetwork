#include "stdafx.h"
#include "Network/NetworkPlugin.h"

DebugCallback g_debugCallback = [](const char* str) { std::cout << str << '\n'; };

bool NetworkAPI::isInitialized = false;

bool NetworkAPI::InitializeSockets()
{
    if(!isInitialized)
    {
        WSADATA WsaData;
        return WSAStartup(MAKEWORD(2, 2), &WsaData) == NO_ERROR;
    }
    return true;
}

void NetworkAPI::ShutdownSockets()
{
    if(isInitialized)
    {
    if (WSACleanup() == SOCKET_ERROR)
        g_debugCallback(("Failed closing Winsock! ERROR_CODE: " + std::to_string(WSAGetLastError())).c_str());
    }
}

int NetworkAPI::SocketGetLastError()
{
    return WSAGetLastError();
}

extern "C"
{
#pragma region Global functions
    
    bool InitializeSockets()
    {
        return NetworkAPI::InitializeSockets();
    }
    
    void ShutdownSockets()
    {
        return NetworkAPI::ShutdownSockets();
    }

    int SocketGetLastError()
    {
        return NetworkAPI::SocketGetLastError();
    }

    void RegisterDebugCallback(DebugCallback p_callback)
    {
        if (p_callback)
            g_debugCallback = p_callback;
    }
}
#pragma endregion