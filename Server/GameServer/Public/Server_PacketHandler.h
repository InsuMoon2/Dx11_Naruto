#pragma once

NS_BEGIN(Server)

class GameSession;

enum PacketID
{
    // Server -> Client
    S_Test          = 1,
    S_EnterGame     = 2,
    S_MyPlayer      = 3,
    S_AddObject     = 4,
    S_RemoveObject  = 5,
    S_Move          = 6,
                       
    // Client -> Server
    C_Move          = 50,

};

struct BuffData
{
    uint64  buffId = 0;
    float   remainTime = 0;
};

class Server_PacketHandler
{
public:
    Server_PacketHandler() = default;
    ~Server_PacketHandler() = default;

public:
    static void HandlePacket(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    // 받기
    static void Handle_C_Move(shared_ptr<GameSession> session, BYTE* buffer, int32 len);

    // 보내기
    static SendBufferRef Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs);
    static SendBufferRef Make_S_MyPlayer(uint64 playerId);
    static SendBufferRef Make_S_AddObject(uint64 playerId);
    static SendBufferRef Make_S_RemoveObject(uint64 playerId);
    static SendBufferRef Make_S_Move(Protocol::C_Move& recvPkt, uint64 playerId);

    template<typename T>
    static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId)
    {
        const uint16 dataSize = static_cast<uint16>(pkt.ByteSizeLong());
        const uint16 packetSize = dataSize + sizeof(PacketHeader);

        SendBufferRef sendBuffer = make_shared<SendBuffer>(packetSize);
        PacketHeader* header = reinterpret_cast<PacketHeader*>(sendBuffer->Buffer());
        header->size = packetSize;
        header->id = pktId;
        assert(pkt.SerializeToArray(&header[1], dataSize));
        sendBuffer->Close(packetSize);

        return sendBuffer;
    }
};

NS_END
