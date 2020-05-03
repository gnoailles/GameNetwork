#pragma once
#include "export.h"

typedef void(__stdcall * DebugCallback) (const char * str);
extern DebugCallback g_debugCallback;

using ShortSharedKey = std::array<uint8_t, Cryptography::Hash::SHA256::OUTPUT_SIZE>;

class NetworkAPI
{
    static bool isInitialized;
public: 
    static bool InitializeSockets();
    static void ShutdownSockets();
    static int  SocketGetLastError();
};

extern "C"
{
    NETWORK_PLUGIN_API bool InitializeSockets();;
    NETWORK_PLUGIN_API void ShutdownSockets();
    NETWORK_PLUGIN_API int SocketGetLastError();
    NETWORK_PLUGIN_API void RegisterDebugCallback(DebugCallback p_callback);
}