#pragma once
#include "stdafx.h"
#include "Address.h"
#include "Socket.h"
#include "Network/NetworkPlugin.h"

struct ChallengeResponsePacket;
struct ConnectionRequestPacket;
struct ConnectionDataPacket;
struct DisconnectPacket;
typedef void(__stdcall * ClientConnectCallback) (int id);

enum class ServerState : uint8_t
{
    LOBBY,
    GAME
};

class Server
{
private:
    using clock = std::chrono::high_resolution_clock;

    struct ChallengeInfo
    {
        Address                     clientAddress;
        NGMP<PUBLIC_KEY_SIZE>       clientPublicKey     {};
        NGMP<PUBLIC_KEY_SIZE>       serverPublicKey     {};
        NGMP<PRIVATE_KEY_SIZE>      serverPrivateKey    {};
        std::future<ShortSharedKey> sharedKeyFuture     {};
        ShortSharedKey              sharedKey           {};
        clock::time_point           lastReceivedPacket  {clock::now()};
    };

    struct ConnectionInfo
    {
        Address             clientAddress;
        ShortSharedKey      sharedKey           {};
        clock::time_point   lastReceivedPacket  {clock::now()};
        uint16_t            sequence            {0};
        uint16_t            ack                 {0};
    };

    static const int                            MAX_CLIENTS                     {4};
    static const int                            TIMEOUT_TIME                    {4};
    static const unsigned short                 SERVER_PORT                     {8755};

    std::random_device                          m_random                        {};
    std::uniform_int_distribution<uint64_t>     m_saltDistribution              {};


    std::array<ChallengeInfo, MAX_CLIENTS>      m_challenges                    {};
    std::array<ConnectionInfo, MAX_CLIENTS>     m_connections                   {};
    bool                                        m_connected[MAX_CLIENTS]        {false};
    bool                                        m_challenged[MAX_CLIENTS]       {false};

    ClientConnectCallback                       m_clientConnectCallback         {nullptr};
    Socket                                      m_socket                        {};
    int                                         m_numConnections                {0};
    std::atomic<ServerState>                    m_state                         {ServerState::LOBBY};


    int                     FindFreeConnectionIndex() const;
    int                     FindFreeChallengeIndex() const;
    int                     FindExistingConnectionIndex(const ConnectionInfo& p_connectionInfo) const;
    int                     FindExistingConnectionIndex(const Address& p_address) const;
    int                     FindExistingChallengeIndex(const Address& p_address) const;

    bool                    IsClientConnected(unsigned int p_clientIndex) const;
    const ConnectionInfo&   GetClientConnectionInfo(unsigned int p_clientIndex) const;

    void                    HandlePacket(const ConnectionRequestPacket& p_packet,  const Address& p_sender);
    void                    HandlePacket(const ChallengeResponsePacket& p_packet,  const Address& p_sender);

    void                    CheckForTimeouts();
    void                    KickClient(const Address& p_address);
    void                    RemoveClient(const Address& p_address);

public:
    Server();
    ~Server();

    int  Listen(unsigned char* o_gameData, unsigned int p_size);
    int  GetConnectedClientCount() const;
    void SwitchToLobby();
    void SwitchToGame();
    void RegisterDebugCallback(ClientConnectCallback p_callback);
    void PropagateGameData(unsigned char* p_buffer, unsigned int p_size);
};

#pragma region CExport
extern "C"  
{
    NETWORK_PLUGIN_API Server*  Internal_ServerCreate();
    NETWORK_PLUGIN_API void     Internal_ServerDestroy(Server* p_obj);
    NETWORK_PLUGIN_API int      Internal_ServerListen(Server* p_obj, unsigned char* o_gameData, unsigned int p_size);

    NETWORK_PLUGIN_API int      Internal_ServerGetConnectedClientCount(Server* p_obj);
    NETWORK_PLUGIN_API void     Internal_ServerSwitchToLobby(Server* p_obj);
    NETWORK_PLUGIN_API void     Internal_ServerSwitchToGame(Server* p_obj);

    NETWORK_PLUGIN_API void     Internal_ServerRegisterClientConnectCallback(Server* p_obj, ClientConnectCallback p_callback);
    NETWORK_PLUGIN_API void     Internal_ServerPropagateGameData(Server* p_obj, unsigned char* p_buffer, unsigned int p_size);
}
#pragma endregion 