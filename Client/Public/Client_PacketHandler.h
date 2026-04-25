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
    S_LobbySnapshot = 7,
    S_LobbyChat     = 8,
    S_LobbyStartGame = 9,
    S_WaveCleared   = 10,
    S_WaveStarted   = 11,

    C_EnterGame     = 49,
    C_Move          = 50,
    C_LobbyJoin     = 51,
    C_LobbyChat     = 52,
    C_LobbyStartGame = 53,
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
    static void Handle_S_WaveCleared(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_WaveStarted(Shared<ServerSession> session, BYTE* buffer, int32 len);

    // 로비 참가자 목록을 받기
    static void Handle_S_LobbySnapshot(Shared<ServerSession> session, BYTE* buffer, int32 len);

    // 로비 채팅 메시지를 받기
    static void Handle_S_LobbyChat(Shared<ServerSession> session, BYTE* buffer, int32 len);
    static void Handle_S_LobbyStartGame(Shared<ServerSession> session, BYTE* buffer, int32 len);

	// 보내기
	static SendBufferRef Make_C_Move(const Protocol::ObjectInfo& objectInfo);
    static SendBufferRef Make_C_EnterGame(const Vec3& spawnPos, float rotY);

    static SendBufferRef Make_C_LobbyJoin();
    static SendBufferRef Make_C_LobbyChat(const wstring& message);
    static SendBufferRef Make_C_LobbyStartGame();

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

        // Release 빌드에서도 protobuf body가 반드시 버퍼에 기록되도록 명시적으로 직렬화한다.
        const bool serialized = pkt.SerializeToArray(&header[1], dataSize);
        assert(serialized);
        if (!serialized)
            return nullptr;

		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

    template<typename T>
    static void ParsePacket(BYTE* buffer, T& outPkt)
    {
        PacketHeader* header = reinterpret_cast<PacketHeader*>(buffer);

        outPkt.ParseFromArray(&header[1], header->size - sizeof(PacketHeader));
    }


private:
    static Shared<GameObject> Spawn_NetworkObject(const Protocol::ObjectInfo& info, uint32 levelIndex);
    static void Apply_NetworkObjectInfo(Shared<GameObject> gameObject, const Protocol::ObjectInfo& info);

};

NS_END
