#include "stdafx.h"
#include "Network/Server.h"
#include "Network/Packets/Buffer.h"
#include "Network/Packets/Packet.h"

using namespace Cryptography;

Server::Server()
{
    if (!m_socket.Open(SERVER_PORT))
        g_debugCallback("Unable to open server socket");

    m_connected[0] = true;
    m_connections[0] = ConnectionInfo{ Address{127,0,0,1,SERVER_PORT} };
    ++m_numConnections;
    if(m_clientConnectCallback != nullptr)
        m_clientConnectCallback(0);
}

Server::~Server()
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (m_connected[i])
           KickClient(m_connections[i].clientAddress);
        if (m_challenged[i])
           KickClient(m_challenges[i].clientAddress);
    }
    m_socket.Close();
}

int Server::FindFreeConnectionIndex() const
{
    for (int i = 0; i < MAX_CLIENTS; ++i)
    {
        if (!m_connected[i])
            return i;
    }
    return -1;
}

int Server::FindFreeChallengeIndex() const
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (!m_challenged[i])
            return i;
    }
    return -1;
}


int Server::FindExistingConnectionIndex(const Address& p_address) const
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_connected[i] && 
            m_connections[i].clientAddress == p_address)
            return i;
    }
    return -1;
}

int Server::FindExistingConnectionIndex(const ConnectionInfo& p_connectionInfo) const
{
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        if (m_connected[i] && 
            m_connections[i].clientAddress == p_connectionInfo.clientAddress &&
            m_connections[i].sharedKey == p_connectionInfo.sharedKey) //TODO probably need to change this to a memcompare
            return i;
    }
    return -1;
}

int Server::FindExistingChallengeIndex(const Address& p_address) const
{
    for (int i = 0; i < m_challenges.size(); ++i)
    {
        if (m_challenged[i] && m_challenges[i].clientAddress == p_address)
            return i;
    }
    return -1;
}

bool Server::IsClientConnected(const unsigned int p_clientIndex) const
{
    if (p_clientIndex > MAX_CLIENTS)
        g_debugCallback("Client index out of range");

    return  m_connected[p_clientIndex];
}

const Server::ConnectionInfo& Server::GetClientConnectionInfo(unsigned int p_clientIndex) const
{
    if (p_clientIndex > MAX_CLIENTS)
        g_debugCallback("Client index out of range");

    return m_connections[p_clientIndex];
}


int Server::Listen(unsigned char* o_gameData, const unsigned int p_size)
{
    try
    {
        Buffer buffer(1024);
        Address sender;
        buffer.size = m_socket.Receive(sender, buffer.data, buffer.size);

        PacketType packetType = PacketType::INVALID_PACKET;
        auto connectionIndex = FindExistingConnectionIndex(sender);
        if (connectionIndex > 0)
        {
            packetType = Packet::VerifyPacketHMAC(m_connections[connectionIndex].sharedKey, buffer);
        }
        else
        {
            auto challengeIndex = FindExistingChallengeIndex(sender);
            if (challengeIndex < 0)
            {
                if (Packet::VerifyPacketCRC(buffer) != PacketType::CONNECTION_REQUEST)
                {
                    return 0;
                }
                else
                {
                    packetType = PacketType::CONNECTION_REQUEST;
                }
            }
            else
            {
                auto& challenge = m_challenges[challengeIndex];
                if (challenge.sharedKeyFuture.valid())
                {
                    challenge.sharedKey = challenge.sharedKeyFuture.get();
                }
                packetType = Packet::VerifyPacketHMAC(challenge.sharedKey, buffer);
            }
        }
        
        switch (packetType) 
        {
            case PacketType::CONNECTION_REQUEST:
            {
                if(m_state.load() == ServerState::LOBBY)
                {
                    ConnectionRequestPacket connectionRequestInfo{};
                    connectionRequestInfo.Read(buffer);
                    HandlePacket(connectionRequestInfo, sender);
                }
                break;
            }
            case PacketType::CHALLENGE_RESPONSE: 
            {
                if(m_state.load() == ServerState::LOBBY)
                {
                    ChallengeResponsePacket challengeResponseInfo{};
                    challengeResponseInfo.Read(buffer);
                    HandlePacket(challengeResponseInfo, sender);
                }
                break;
            }
            case PacketType::CONNECTION_DATA:
            {
                if(m_state.load() == ServerState::GAME)
                {
                    ConnectionDataPacket connectionDataInfo{};
                    connectionDataInfo.Read(buffer);
                    const int clientIdx = FindExistingConnectionIndex(sender);
                    if (clientIdx > 0)
                    {
                        if(static_cast<int>(p_size) >= buffer.size + sizeof(int))
                        {
                            *reinterpret_cast<int*>(o_gameData) = clientIdx;
                            memcpy(o_gameData + sizeof(int), connectionDataInfo.gameData, connectionDataInfo.gameDataSize);
                            return connectionDataInfo.gameDataSize;
                        }
                        else
                        {
                            g_debugCallback("Buffer too small for Game Data");
                            return -1;
                        }
                    }
                }
                break;
            }
            case PacketType::DISCONNECT: 
            {
                DisconnectPacket disconnectInfo{};
                disconnectInfo.Read(buffer);
                const int clientIdx = FindExistingConnectionIndex(sender);
                const int clientChallIdx = FindExistingChallengeIndex(sender);
                if ((clientIdx > 0) || 
                    (clientChallIdx > 0))
                    RemoveClient(sender);
                break;
            }
            default: return 0;
        }
        CheckForTimeouts();
    }
    catch(std::exception& e)
    {
        g_debugCallback(e.what());
    }
    return 0;
}

int Server::GetConnectedClientCount() const
{
    return m_numConnections;
}

void Server::SwitchToLobby()
{
    m_state.store(ServerState::LOBBY);
}

void Server::SwitchToGame()
{
    m_state.store(ServerState::GAME);
    for(int i = 1; i < MAX_CLIENTS; i++)
    {
        if(m_connected[i])
            m_connections[i].lastReceivedPacket = clock::now();
    }
}

void Server::RegisterDebugCallback(ClientConnectCallback p_callback)
{
    if(p_callback)
        m_clientConnectCallback = p_callback;
}


void Server::PropagateGameData(unsigned char* p_buffer, unsigned int p_size)
{
    for (int i = 1; i < MAX_CLIENTS; i++)
    {
        if(m_connected[i])
        {
            ConnectionInfo& connectionInfo = m_connections[i];
            Buffer packet;
            ConnectionDataPacket packetInfo{++connectionInfo.sequence, p_size,  new unsigned char[p_size]};
            memcpy(packetInfo.gameData, p_buffer, p_size);
            packetInfo.Write(packet, connectionInfo.sharedKey);

            if(!m_socket.Send(connectionInfo.clientAddress, packet.data, packet.size))
            {
                g_debugCallback(std::string("Server failed to send GameData packet to client:" + std::to_string(i)).c_str());
            }
        }
    }
}

void Server::HandlePacket(const ConnectionRequestPacket& p_packet, const Address& p_sender)
{
    int challIndex = FindExistingChallengeIndex(p_sender);
    if(challIndex < 0)
    {
        if((challIndex = FindFreeChallengeIndex() >= 0))
        {
            m_challenges[challIndex] = { p_sender, p_packet.clientPublicKey };
            KeyExchange::DiffieHellman::GenerateKeyPair(m_challenges[challIndex].serverPrivateKey, m_challenges[challIndex].serverPublicKey);
            m_challenged[challIndex] = true;
        }
        else
        {
            g_debugCallback("Server full, connection denied");
            return;
        }
    }
    
    m_challenges[challIndex].sharedKeyFuture = std::async(std::launch::async, [&]() 
    { 
        auto sharedKey = KeyExchange::DiffieHellman::GenerateSharedKey(m_challenges[challIndex].clientPublicKey, m_challenges[challIndex].serverPrivateKey); 
        return Hash::SHA256().Hash(reinterpret_cast<unsigned char*>(sharedKey.Get64BitArray()), PUBLIC_KEY_SIZE / 8);
    });

    Buffer challenge;
    ChallengePacket packetInfo {m_challenges[challIndex].serverPublicKey};

    packetInfo.Write(challenge);
    if(!m_socket.Send(p_sender, challenge.data, challenge.size))
        g_debugCallback("Server failed to send Challenge packet");
    else
        g_debugCallback("Server sent Challenge packet");
}

void Server::HandlePacket(const ChallengeResponsePacket& p_packet, const Address& p_sender)
{
    g_debugCallback("Server Handling Challenge Response");
    const int challengeIndex = FindExistingChallengeIndex(p_sender);
    if(challengeIndex >= 0)
    {
        //const ChallengeInfo connection = m_challenges[challengeIndex];
        const int newClientIndex = FindFreeConnectionIndex();

        auto& challenge = m_challenges[challengeIndex];
        if (challenge.sharedKeyFuture.valid())
        {
            challenge.sharedKey = challenge.sharedKeyFuture.get();
        }

        if (newClientIndex > -1)
        {
            m_connected[newClientIndex] = true;
            m_connections[newClientIndex] = { challenge.clientAddress, challenge.sharedKey, clock::now() };
            ++m_numConnections;

            m_challenged[challengeIndex] = false;
            m_challenges[challengeIndex] = {};

            Buffer accepted;
            ConnectionAcceptedPacket packetInfo { static_cast<uint32_t>(newClientIndex) };

            if(m_clientConnectCallback)
                m_clientConnectCallback(newClientIndex);
            packetInfo.Write(accepted, m_connections[newClientIndex].sharedKey);
            if(!m_socket.Send(p_sender,accepted.data, accepted.size))
                g_debugCallback("Server failed to send Connection Accepted packet");
            g_debugCallback(("New client connected, ID: "+ std::to_string(newClientIndex), " Address: "+  m_connections[newClientIndex].clientAddress.ToString()).c_str());
        }
    }
}

void Server::CheckForTimeouts()
{
    //(DEBUG) Disabling timeout while testing KeyExchange
    //for (int i = 0; i < MAX_CLIENTS; ++i)
    //{
    //    if (m_challenged[i] && clock::now() - m_challenges[i].lastReceivedPacket > std::chrono::seconds(TIMEOUT_TIME))
    //        KickClient(m_challenges[i].clientAddress);
    //}
    //(DEBUG) Disabling In-Game timeout for debug
    // if(m_state.load() == ServerState::GAME)
    // {
    //     for (int i = 1; i < MAX_CLIENTS; ++i)
    //     {
    //         if(m_connected[i] && clock::now() - m_connections[i].lastReceivedPacket > std::chrono::seconds(TIMEOUT_TIME))
    //             KickClient(m_connections[i].clientAddress);
    //     }
    // }
}

void Server::KickClient(const Address& p_address)
{
    const int clientIdx = FindExistingConnectionIndex(p_address);
    const int clientChallIdx = FindExistingChallengeIndex(p_address);
    ShortSharedKey sharedKey;
    Address clientAddress;

    if (clientIdx > 0)
    {
        sharedKey = m_connections[clientIdx].sharedKey;
        clientAddress = m_connections[clientIdx].clientAddress;
    }
    else if (clientChallIdx > 0)
    {
        auto& challenge = m_challenges[clientChallIdx];
        if (challenge.sharedKeyFuture.valid())
        {
            challenge.sharedKey = challenge.sharedKeyFuture.get();
        }
        sharedKey = challenge.sharedKey;
        clientAddress = challenge.clientAddress;
    }
    else
        return;

    Buffer packet;
    DisconnectPacket packetInfo {};
    packetInfo.Write(packet, sharedKey);
    for(int i = 0; i < 10; ++i)
    {
        if(!m_socket.Send(clientAddress, packet.data, packet.size))
        {
            g_debugCallback("Server failed to send Disconnect packet");
        }
    }
    g_debugCallback("Server sent Disconnect packet");
    RemoveClient(p_address);
}

void Server::RemoveClient(const Address& p_address)
{
    if(const int clientIdx = FindExistingConnectionIndex(p_address) > 0)
    {
        m_connected[clientIdx] = false;
        m_connections[clientIdx] = {};
        --m_numConnections;
    }
    else if(const int challIdx = FindExistingChallengeIndex(p_address) >= 0)
    {
        m_challenged[challIdx] = false;
        m_challenges[challIdx] = {};
    }
}

#pragma region CExport
extern "C"
{
    Server* Internal_ServerCreate()
    {
        return new Server;
    }

    void Internal_ServerDestroy(Server* p_obj)
    {
        if(p_obj != NULL)
            delete p_obj;
    }

    int Internal_ServerListen(Server* p_obj, unsigned char* o_gameData, unsigned int p_size)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }

        return p_obj->Listen(o_gameData, p_size);
    }

    int Internal_ServerGetConnectedClientCount(Server* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }

        return p_obj->GetConnectedClientCount();
    }

    void Internal_ServerSwitchToLobby(Server* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }

        p_obj->SwitchToLobby();
    }

    void Internal_ServerSwitchToGame(Server* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }
        p_obj->SwitchToGame();
    }

    void Internal_ServerRegisterClientConnectCallback(Server* p_obj, ClientConnectCallback p_callback)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }
        p_obj->RegisterDebugCallback(p_callback);
    }
    void Internal_ServerPropagateGameData(Server* p_obj, unsigned char* p_buffer, const unsigned int p_size)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }
        p_obj->PropagateGameData(p_buffer, p_size);
    }
}
#pragma endregion 
