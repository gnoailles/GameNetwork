#include "stdafx.h"
#include "Network/Client.h"
#include "Network/Packets/Buffer.h"
#include "Network/Packets/Packet.h"
#include "Network/Address.h"
#include "Network/NetworkPlugin.h"

using namespace std::chrono_literals;
using namespace Cryptography;

Client::Client()
{
    SetupBroadcastSocket();
}


Client::~Client()
{
    if(m_state == ClientState::CONNECTED)
        SendDisconnect();
    m_socket.Close();
}


void Client::SetupBroadcastSocket()
{
    if (!m_socket.Open(CLIENT_PORT) || !m_socket.AllowBroadcast())
    {
        g_debugCallback("Unable to open client socket");
    }
}

void Client::Connect()
{
    try
    {
        if(m_state.load() == ClientState::DISCONNECTED)
        {
            Buffer packet;
            KeyExchange::DiffieHellman::GenerateKeyPair(m_privateKey, m_publicKey);
            //m_salt = m_saltDistribution(m_random);
            ConnectionRequestPacket packetInfo {m_publicKey};
            packetInfo.Write(packet);
            m_state.store(ClientState::SENDING_REQUEST);

            const clock::time_point start = clock::now();
            //while(m_state.load() == ClientState::SENDING_REQUEST)
            {
                if(!m_socket.Send({INADDR_BROADCAST, SERVER_PORT}, packet.data, packet.size))
                    g_debugCallback("Failed to send connection request packet");
                else
                    g_debugCallback("Client sent connection request packet");
                /*if((std::chrono::high_resolution_clock::now() - start) > 5s)
                {
                    g_debugCallback("Connection attempt timed out after 5s");
                    Disconnect();
                    return;
                }*/
                //std::this_thread::sleep_for(1000ms);
            }
        }
    }
    catch(std::exception& e)
    {
        g_debugCallback(e.what());
    }

}

void Client::RespondChallenge()
{
    try
    {
        Buffer packet;
        ChallengeResponsePacket packetInfo;
        packetInfo.Write(packet, m_sharedKey);
        m_state.store(ClientState::SENDING_CHALLENGE_RESPONSE);
        const clock::time_point start = clock::now();
        //while(m_state.load() == ClientState::SENDING_CHALLENGE_RESPONSE)
        {
            g_debugCallback(m_serverAddress.ToString().c_str());
            if(!m_socket.Send(m_serverAddress, packet.data, packet.size))
                g_debugCallback("Failed to send Challenge Response packet");
            else
                g_debugCallback("Client sent Challenge Response packet");

            /*if((std::chrono::high_resolution_clock::now() - start) > 5s)
            {
                g_debugCallback("Respond Challenge attempt timed out after 5s");
                SendDisconnect();
                return;
            }*/
            //std::this_thread::sleep_for(1000ms);
        }
    }
    catch(std::exception& e)
    {
        g_debugCallback(e.what());
    }
}

void Client::SendDisconnect()
{
    try
    {
        if(m_state.load() != ClientState::DISCONNECTED && m_state.load() != ClientState::SENDING_REQUEST)
        {
            Buffer packet;
            DisconnectPacket packetInfo;
            packetInfo.Write(packet, m_sharedKey);
            for(int i = 0; i < 10; ++i)
            {
                if(!m_socket.Send(m_serverAddress, packet.data, packet.size))
                    g_debugCallback("Failed to send Disconnect packet");
            }
            g_debugCallback("Client sent Disconnect packets");
        }
        Disconnect();
    }
    catch(std::exception& e)
    {
        g_debugCallback(e.what());
    }
}

void Client::HandlePacket(const ChallengePacket& p_packet)
{
    //if(p_packet.clientSalt != m_salt)
    //    return;
    auto sharedKey = KeyExchange::DiffieHellman::GenerateSharedKey(p_packet.serverPublicKey, m_privateKey);
    m_sharedKey = Hash::SHA256().Hash(reinterpret_cast<unsigned char*>(sharedKey.Get64BitArray()), PUBLIC_KEY_SIZE / 8);
    std::thread respondThread(&Client::RespondChallenge, this);
    respondThread.detach();
}

void Client::HandlePacket(const ConnectionAcceptedPacket& p_packet)
{
    m_index = p_packet.clientID;
    m_state.store(ClientState::CONNECTED);
    g_debugCallback("Client is connected");
}

void Client::Disconnect()
{
    memset(m_sharedKey.data(), 0, m_sharedKey.size());
    m_serverAddress = {};
    m_state.store(ClientState::DISCONNECTED);
}

int Client::Listen(unsigned char* o_gameData, const unsigned int p_size)
{
    Buffer buffer(1024);
    buffer.size = m_socket.Receive(m_serverAddress,buffer.data, buffer.size);

    PacketType packetType = PacketType::INVALID_PACKET;
    if (m_state.load() == ClientState::DISCONNECTED || m_state.load() == ClientState::SENDING_REQUEST)
    {
        packetType = Packet::VerifyPacketCRC(buffer);
    }
    else
    {
        packetType = Packet::VerifyPacketHMAC(m_sharedKey, buffer);
    }

    if(packetType != PacketType::INVALID_PACKET)
        m_lastReceivedPacket = clock::now();

    switch (packetType)
    {
        case PacketType::CHALLENGE:
        {
            if(m_state.load() == ClientState::SENDING_REQUEST)
            {
                g_debugCallback("Client received CHALLENGE packet");
                ChallengePacket packetInfo{};
                packetInfo.Read(buffer);
                HandlePacket(packetInfo);
            }
            break;
        }
        case PacketType::CONNECTION_ACCEPTED: 
        {
            if(m_state.load() == ClientState::SENDING_CHALLENGE_RESPONSE)
            {
                g_debugCallback("Client received CONNECTION_ACCEPTED packet");
                ConnectionAcceptedPacket packetInfo{};
                packetInfo.Read(buffer);
                HandlePacket(packetInfo);
            }
            break;
        }
        case PacketType::CONNECTION_DATA: 
        {
            if (m_state.load() == ClientState::CONNECTED)
            {
                ConnectionDataPacket packetInfo;
                packetInfo.Read(buffer);
                if (Packet::SequenceGreaterThan(packetInfo.sequence, m_ack))
                {
                    m_ack = packetInfo.sequence;
                    if (static_cast<int>(p_size) >= packetInfo.gameDataSize)
                        memcpy(o_gameData, packetInfo.gameData, packetInfo.gameDataSize);
                    else
                    {
                        g_debugCallback("Buffer too small for Game Data");
                        return -1;
                    }
                    return packetInfo.gameDataSize;
                }
                return 0;
            }
        }
        case PacketType::DISCONNECT:
            if(m_state.load() != ClientState::DISCONNECTED)
            {
                g_debugCallback("Client received DISCONNECT packet");
                Disconnect();
            }
            break;
        default: break;
    }
    if(m_activeTimeout && m_state.load() == ClientState::CONNECTED && clock::now() - m_lastReceivedPacket > 5s)
    {
        g_debugCallback("Client connection timed out!");
        SendDisconnect();
        return 0;
    }
    return 0;
}

bool Client::SendGameData(const unsigned char* p_data, unsigned int p_size)
{
    if(m_state.load() == ClientState::CONNECTED)
    {
        Buffer packet;
        ConnectionDataPacket packetInfo { ++m_sequence, p_size, new unsigned char[p_size] };
        memcpy(packetInfo.gameData, p_data, p_size);
        packetInfo.Write(packet, m_sharedKey);
        if(!m_socket.Send(m_serverAddress, packet.data, packet.size))
        {
            g_debugCallback("Failed to send ConnectionData packet");
            return false;
        }
        return true;
    }
    return false;
}

void Client::SetActiveTimeout(bool p_value)
{
    m_activeTimeout = p_value;
}

int Client::GetIndex() const
{
    return m_index;
}

char Client::GetState() const
{
    return static_cast<char>(m_state.load());
}


#pragma region CExport
extern "C"
{
    Client* Internal_ClientCreate()
    {
        return new Client;
    }

    void Internal_ClientDestroy(Client* p_obj)
    {
        if (p_obj != NULL)
            delete p_obj;
    }

    void Internal_ClientConnect(Client* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }

        return p_obj->Connect();
    }
    void Internal_ClientDisconnect(Client* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }

        return p_obj->SendDisconnect();
    }


    int Internal_ClientListen(Client* p_obj, unsigned char* o_gameData, const unsigned int p_size)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }
        return p_obj->Listen(o_gameData, p_size);
    }

    bool Internal_ClientSendGameData(Client* p_obj, const unsigned char* p_data, unsigned int p_size)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return false;
        }
        return p_obj->SendGameData(p_data, p_size);
    }

    void Internal_ClientSetActiveTimeout(Client* p_obj, bool p_value)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return;
        }
        return p_obj->SetActiveTimeout(p_value);
    }

    int Internal_ClientGetIndex(Client* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }
        return p_obj->GetIndex();
    }

    char Internal_ClientGetState(Client* p_obj)
    {
        if (p_obj == NULL)
        {
            g_debugCallback("Invalid instance pointer!");
            return -1;
        }
        return p_obj->GetState();
    }
}

#pragma endregion
