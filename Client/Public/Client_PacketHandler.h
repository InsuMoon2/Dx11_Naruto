#pragma once

#include "Engine_Macro.h"

NS_BEGIN(Client)

class ServerSession;
class Player;

enum PacketID
{
    S_TEST          = 1,
    S_EnterGame     = 2,
    S_MyPlayer      = 3,
    S_AddObject     = 4,
    S_RemoveObject  = 5,
    S_Move          = 6,

    C_EnterGame     = 49,
    C_Move          = 50,
};

class Client_PacketHandler
{
public:
	static void HandlePacket(Shared<ServerSession> session, BYTE* buffer, int32 len);

	// 받기
	static void Handle_S_TEST(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_MyPlayer(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_AddObject(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_RemoveObject(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_Move(Shared<ServerSession> session, BYTE* buffer, int32 len);

	// 보내기
	static SendBufferRef Make_C_Move(const Protocol::ObjectInfo& objectInfo);
    static SendBufferRef Make_C_EnterGame(const Vec3& spawnPos, float rotY);

public:
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

    template<typename T>
    static void ParsePacket(BYTE* buffer, T& outPkt)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

        outPkt.ParseFromArray(&header[1], header->size - sizeof(PacketHeader));
    }

};

NS_END
