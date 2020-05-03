#include "stdafx.h"
#include "Network/Packets/Packet.h"
#include "Network/ErrorDetection/CRC.h"
#include "Network/Client.h"
#include "Network/ErrorDetection/Checksums.h"

const uint32_t                          Packet::PROTOCOL_ID = Fletcher32_improved(reinterpret_cast<const uint16_t*>("NetworkPluginV1.0.0"), 10);

using namespace Cryptography;

PacketType Packet::VerifyPacketHMAC(const ShortSharedKey& p_key, Buffer& p_buffer)
{
    if (p_buffer.size <= (int)Hash::HMAC::SIZE)
        return PacketType::INVALID_PACKET;


    Buffer message(p_buffer.size - Hash::HMAC::SIZE);
    p_buffer.ReadBuffer(message.data, message.size);
    const auto newHMAC = Hash::HMAC::HMAC_SHA256(p_key.data(), p_key.size(), message.data, message.size);

    Buffer hmac(Hash::HMAC::SIZE);
    p_buffer.ReadBuffer(hmac.data, Hash::HMAC::SIZE);
    p_buffer.index = 0;
    if (memcmp(newHMAC.data(), hmac.data, Hash::HMAC::SIZE) != 0)
    {
        g_debugCallback("Invalid HMAC, Discarded packet!");
        return PacketType::INVALID_PACKET;
    }

    if (p_buffer.ReadInteger() != PROTOCOL_ID)
    {
        g_debugCallback("Invalid Protocol id, Discarded packet!");
        return PacketType::INVALID_PACKET;
    }
    return static_cast<PacketType>(p_buffer.ReadByte());
}

PacketType Packet::VerifyPacketCRC(Buffer& p_buffer)
{
    if(p_buffer.size <= 0)
        return PacketType::INVALID_PACKET;

    CRC32::InitTable();
    const uint32_t crc = p_buffer.ReadInteger();

    p_buffer.index = 0;
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    const uint32_t newCrc = CRC32::GetCRCTableBased(p_buffer.data, p_buffer.size);
    if(crc != newCrc)
    {
        g_debugCallback("Invalid CRC, Discarded packet!");
        return PacketType::INVALID_PACKET;
    }
    return static_cast<PacketType>(p_buffer.ReadByte());
}


#pragma region ConnectionRequestPacket
void ConnectionRequestPacket::Write(Buffer& p_buffer)
{
    CRC32::InitTable();
    p_buffer.Init(Packet::CONNECTION_REQUEST_PACKET_SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::CONNECTION_REQUEST));
    p_buffer.WriteBuffer(reinterpret_cast<unsigned char *>(clientPublicKey.Get64BitArray()), Packet::ROUNDED_PUBLIC_KEY_SIZE);

    const uint32_t crc = CRC32::GetCRCTableBased(p_buffer.data, Packet::CONNECTION_REQUEST_PACKET_SIZE);

    p_buffer.index = 0;
    p_buffer.WriteInteger(crc);
}

void ConnectionRequestPacket::Read(Buffer& p_buffer)
{
    p_buffer.ReadBuffer(reinterpret_cast<unsigned char *>(clientPublicKey.Get64BitArray()), Packet::ROUNDED_PUBLIC_KEY_SIZE);
}
#pragma endregion 

#pragma  region ChallengePacket
void ChallengePacket::Write(Buffer& p_buffer)
{
    CRC32::InitTable();
    p_buffer.Init(Packet::CHALLENGE_PACKET_SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::CHALLENGE));
    p_buffer.WriteBuffer(reinterpret_cast<unsigned char *>(serverPublicKey.Get64BitArray()), Packet::ROUNDED_PUBLIC_KEY_SIZE);

    const uint32_t crc = CRC32::GetCRCTableBased(p_buffer.data, Packet::CHALLENGE_PACKET_SIZE);

    p_buffer.index = 0;
    p_buffer.WriteInteger(crc);
}

void ChallengePacket::Read(Buffer& p_buffer)
{
    p_buffer.ReadBuffer(reinterpret_cast<unsigned char *>(serverPublicKey.Get64BitArray()), Packet::ROUNDED_PUBLIC_KEY_SIZE);
}
#pragma endregion 

#pragma  region ChallengeResponsePacket
void ChallengeResponsePacket::Write(Buffer& p_buffer, const ShortSharedKey& sharedKey)
{
    p_buffer.Init(Packet::CHALLENGE_RESPONSE_PACKET_SIZE + Hash::HMAC::SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::CHALLENGE_RESPONSE));

    auto hmac = Hash::HMAC::HMAC_SHA256(sharedKey.data(), sharedKey.size(), p_buffer.data, Packet::CHALLENGE_RESPONSE_PACKET_SIZE);
    p_buffer.WriteBuffer(hmac.data(), hmac.size());
}

void ChallengeResponsePacket::Read(Buffer& p_buffer)
{
    //p_buffer.ReadBuffer(sharedKey.data(), sharedKey.size());
}
#pragma endregion 




#pragma  region ConnectionAcceptedPacket
void ConnectionAcceptedPacket::Write(Buffer& p_buffer, const ShortSharedKey& sharedKey)
{
    p_buffer.Init(Packet::CONNECTION_ACCEPTED_PACKET_SIZE + Hash::HMAC::SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::CONNECTION_ACCEPTED));
    p_buffer.WriteInteger(clientID);

    auto hmac = Hash::HMAC::HMAC_SHA256(sharedKey.data(), sharedKey.size(), p_buffer.data, Packet::CONNECTION_ACCEPTED_PACKET_SIZE);
    p_buffer.WriteBuffer(hmac.data(), hmac.size());
}


void ConnectionAcceptedPacket::Read(Buffer& p_buffer)
{
    //p_buffer.ReadBuffer(sharedKey.data(), sharedKey.size());
    clientID    = p_buffer.ReadInteger();
}
#pragma endregion 

#pragma  region ConnectionDataPacket
void ConnectionDataPacket::Write(Buffer& p_buffer, const ShortSharedKey& sharedKey)
{
    p_buffer.Init(Packet::CONNECTION_DATA_PACKET_SIZE + gameDataSize + Hash::HMAC::SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::CONNECTION_DATA));
    p_buffer.WriteShort(sequence);
    p_buffer.WriteInteger(gameDataSize);
    p_buffer.WriteBuffer(gameData, gameDataSize);

    auto hmac = Hash::HMAC::HMAC_SHA256(sharedKey.data(), sharedKey.size(), p_buffer.data, Packet::CONNECTION_DATA_PACKET_SIZE + gameDataSize);
    p_buffer.WriteBuffer(hmac.data(), hmac.size());
}

void ConnectionDataPacket::Read(Buffer& p_buffer)
{
    sequence = p_buffer.ReadShort();
    //p_buffer.ReadBuffer(sharedKey.data(), sharedKey.size());
    gameDataSize = p_buffer.ReadInteger();
    gameData = new unsigned char[gameDataSize];
    p_buffer.ReadBuffer(gameData, gameDataSize);
}

ConnectionDataPacket::~ConnectionDataPacket()
{
        delete gameData;
}
#pragma endregion 

#pragma  region DisconnectPacket
void DisconnectPacket::Write(Buffer& p_buffer, const ShortSharedKey& sharedKey)
{
    p_buffer.Init(Packet::DISCONNECT_PACKET_SIZE + Hash::HMAC::SIZE);
    p_buffer.WriteInteger(Packet::PROTOCOL_ID);
    p_buffer.WriteByte(static_cast<uint8_t>(PacketType::DISCONNECT));

    auto hmac = Hash::HMAC::HMAC_SHA256(sharedKey.data(), sharedKey.size(), p_buffer.data, Packet::DISCONNECT_PACKET_SIZE);
    p_buffer.WriteBuffer(hmac.data(), hmac.size());
}

void DisconnectPacket::Read(Buffer& p_buffer)
{
    //p_buffer.ReadBuffer(sharedKey.data(), sharedKey.size());
}
#pragma endregion 

