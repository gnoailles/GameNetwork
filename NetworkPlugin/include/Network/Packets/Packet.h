#pragma once
#include "stdafx.h"
#include "export.h"
#include "Buffer.h"
#include "Network/NetworkPlugin.h"

using namespace Cryptography;

enum class PacketType : uint8_t
{
    INVALID_PACKET,
    CONNECTION_REQUEST,
    CHALLENGE,
    CHALLENGE_RESPONSE,
    CONNECTION_ACCEPTED,
    CONNECTION_DATA,
    DISCONNECT,
};

class Packet
{
public:
    static const    uint32_t                            PROTOCOL_ID;
    static const    uint32_t                            MINIMUM_HEADER_SIZE             = sizeof(PROTOCOL_ID) + sizeof(uint8_t); // ProtocolID + PacketType
    static const    uint32_t                            ROUNDED_PUBLIC_KEY_SIZE         = (PUBLIC_KEY_SIZE + 7) / 8;
    static const    unsigned int                        CONNECTION_REQUEST_PACKET_SIZE  = MINIMUM_HEADER_SIZE + ROUNDED_PUBLIC_KEY_SIZE;
    static const    unsigned int                        CHALLENGE_PACKET_SIZE           = MINIMUM_HEADER_SIZE + ROUNDED_PUBLIC_KEY_SIZE;
    static const    unsigned int                        CHALLENGE_RESPONSE_PACKET_SIZE  = MINIMUM_HEADER_SIZE;
    static const    unsigned int                        CONNECTION_ACCEPTED_PACKET_SIZE = MINIMUM_HEADER_SIZE + 4;
    static const    unsigned int                        CONNECTION_DATA_PACKET_SIZE     = MINIMUM_HEADER_SIZE + 2 + 4;
    static const    unsigned int                        DISCONNECT_PACKET_SIZE          = MINIMUM_HEADER_SIZE;
protected:

    static const    unsigned int                        PREVIOUS_ACK_COUNT              = 32;
    static const    unsigned int                        DATA_PROTOCOL_SIZE              = sizeof(unsigned int);
    static const    unsigned int                        SEQUENCE_SIZE                   = sizeof(int);
    static const    unsigned int                        HEADER_SIZE                     = 2 * DATA_PROTOCOL_SIZE + 2 * SEQUENCE_SIZE;



    static          int                                 sequence;
    static          int                                 ack;
    static          std::bitset<PREVIOUS_ACK_COUNT>     previousAck;


    /**
     * Compare sequence ID handling wrap around
     */

public:
    Packet() = delete;
    ~Packet() = delete;
    static PacketType VerifyPacketHMAC(const ShortSharedKey& p_key, Buffer& p_buffer);
    static PacketType VerifyPacketCRC(Buffer& p_buffer);

    template<typename T>
    static bool SequenceGreaterThan(T p_sequence1, T p_sequence2)
    {
        return  ((p_sequence1 > p_sequence2) && (p_sequence1 - p_sequence2 <= std::numeric_limits<T>::max() * 0.5f)) ||
                ((p_sequence1 < p_sequence2) && (p_sequence2 - p_sequence1 >  std::numeric_limits<T>::max() * 0.5f));
    }

};


struct ConnectionRequestPacket
{
    NGMP<PUBLIC_KEY_SIZE>    clientPublicKey  = 0;

    void Write(Buffer& p_buffer);
    void Read(Buffer& p_buffer);
};

struct ChallengePacket
{
    NGMP<PUBLIC_KEY_SIZE>    serverPublicKey  = 0;

    void Write(Buffer& p_buffer);
    void Read(Buffer& p_buffer);
};

struct ChallengeResponsePacket
{
    void Write(Buffer& p_buffer, const ShortSharedKey& sharedKey);
    void Read(Buffer& p_buffer);
};

struct ConnectionAcceptedPacket
{
    uint32_t    clientID    = 0;

    void Write(Buffer& p_buffer, const ShortSharedKey& sharedKey);
    void Read(Buffer& p_buffer);
};

struct ConnectionDataPacket
{
    uint16_t        sequence        = 0;
    uint32_t        gameDataSize    = 0;
    unsigned char*  gameData        = nullptr;

    void Write(Buffer& p_buffer, const ShortSharedKey& sharedKey);
    void Read(Buffer& p_buffer);

    ~ConnectionDataPacket();
};
struct DisconnectPacket
{
    void Write(Buffer& p_buffer, const ShortSharedKey& sharedKey);
    void Read(Buffer& p_buffer);
};