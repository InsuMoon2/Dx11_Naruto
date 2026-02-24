#pragma once

#include "Engine_Macro.h"

NS_BEGIN(Client)

class ServerSession;

enum PacketID
{
    S_TEST          = 1,
    S_EnterGame     = 2,
    S_MyPlayer      = 3,
    S_AddObject     = 4,
    S_RemoveObject  = 5,
    S_Move          = 6,

    C_Move          = 50,
};

class Client_PacketHandler
{
public:
	static void HandlePacket(shared_ptr<ServerSession> session, BYTE* buffer, int32 len);

	// 받기
	static void Handle_S_TEST(shared_ptr<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_Move(shared_ptr<ServerSession> session, BYTE* buffer, int32 len);

	// 보내기
	static SendBufferRef Make_C_Move(float x, float y, float z, float rotY);

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
