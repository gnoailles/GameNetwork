#pragma once
#include "stdafx.h"
#include "Socket.h"
#include "Address.h"
#include "NetworkPlugin.h"


struct ConnectionAcceptedPacket;
struct ChallengePacket;
struct DisconnectPacket;

enum class ClientState : uint8_t
{
    DISCONNECTED,
    SENDING_REQUEST,
    SENDING_CHALLENGE_RESPONSE,
    CONNECTED
};

class Client
{
private:
    using clock = std::chrono::high_resolution_clock;

    static const uint16_t                   CLIENT_PORT         {0};
    static const uint16_t                   SERVER_PORT         {8755};

    std::random_device                      m_random            {};
    std::uniform_int_distribution<uint64_t> m_saltDistribution  {};

    NGMP<PRIVATE_KEY_SIZE>                  m_privateKey        {};
    NGMP<PUBLIC_KEY_SIZE>                   m_publicKey         {};

    ShortSharedKey                          m_sharedKey         {};
    //uint64_t                                m_salt              {0};
    //uint64_t                                m_serverSalt        {0};
    Address                                 m_serverAddress     {};
    Socket                                  m_socket            {};
    int                                     m_index             {-1};
    std::atomic<ClientState>                m_state             {ClientState::DISCONNECTED};
    clock::time_point                       m_lastReceivedPacket;
    uint16_t                                m_sequence          {0};
    uint16_t                                m_ack               {0};
    bool                                    m_activeTimeout     {false};

    void SetupBroadcastSocket();
    void RespondChallenge();
    void HandlePacket(const ChallengePacket& p_packet);
    void HandlePacket(const ConnectionAcceptedPacket& p_packet);
    void Disconnect();
public:
    Client();
    ~Client();
    void Connect();
    void SendDisconnect();
    int Listen(unsigned char* o_gameData, unsigned int p_size);
    bool SendGameData(const unsigned char* p_data, unsigned int p_size);

    void SetActiveTimeout(bool p_value);

    int  GetIndex() const;
    char GetState() const;
};

#pragma region CExport
extern "C"
{
    NETWORK_PLUGIN_API Client*  Internal_ClientCreate();
    NETWORK_PLUGIN_API void     Internal_ClientDestroy(Client* p_obj);
    NETWORK_PLUGIN_API void     Internal_ClientConnect(Client* p_obj);
    NETWORK_PLUGIN_API void     Internal_ClientDisconnect(Client* p_obj);
    NETWORK_PLUGIN_API int      Internal_ClientListen(Client* p_obj, unsigned char* o_gameData, unsigned int p_size);
    NETWORK_PLUGIN_API bool     Internal_ClientSendGameData(Client* p_obj, const unsigned char* p_data, unsigned int p_size);

    NETWORK_PLUGIN_API void     Internal_ClientSetActiveTimeout(Client* p_obj, bool p_value);

    NETWORK_PLUGIN_API int      Internal_ClientGetIndex(Client* p_obj);
    NETWORK_PLUGIN_API char     Internal_ClientGetState(Client* p_obj);
}
#pragma endregion