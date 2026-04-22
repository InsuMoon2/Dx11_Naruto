#include "pch.h"
#include "Server_PacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameRoom.h"
#include "GameSession.h"

void Server_PacketHandler::HandlePacket(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    BufferReader reader(buffer, len);
    PacketHeader header;
    reader.Peek(&header);

    switch (header.id)
    {
    case C_Move:
        Handle_C_Move(session, buffer, len);
        break;

    case C_EnterGame:
        Handle_C_EnterGame(session, buffer, len);
        break;

    case C_LobbyJoin:
        Handle_C_LobbyJoin(session, buffer, len);
        break;

    case C_LobbyChat:
        Handle_C_LobbyChat(session, buffer, len);
        break;

    case C_LobbyStartGame:
        Handle_C_LobbyStartGame(session, buffer, len);
        break;

    default:
        break;
    }

}

void Server_PacketHandler::Handle_C_Move(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_Move pkt;
    ParsePacket(buffer, pkt);

    // [변경] 플레이어 이동만 세션의 playerId로 강제하고, 몬스터는 권한 클라이언트가 보낸 objectId를 유지한다.
    if (pkt.info().objecttype() != Protocol::OBJECT_TYPE_MONSTER &&
        pkt.info().objecttype() != Protocol::OBJECT_TYPE_BOSS_PAIN)
    {
        pkt.mutable_info()->set_objectid(session->Get_PlayerId());
    }

    GRoom->Handle_C_Move(session, pkt);
}

void Server_PacketHandler::Handle_C_EnterGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_EnterGame pkt;
    ParsePacket(buffer, pkt);

    GRoom->Enter_GameRoom(session, pkt);
}

void Server_PacketHandler::Handle_C_LobbyJoin(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyJoin pkt;
    ParsePacket(buffer, pkt);

    GRoom->Join_Lobby(session, pkt);
}

void Server_PacketHandler::Handle_C_LobbyChat(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyChat pkt;
    ParsePacket(buffer, pkt);

    GRoom->Handle_LobbyChat(session, pkt);
}

void Server_PacketHandler::Handle_C_LobbyStartGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_LobbyStartGame pkt;
    ParsePacket(buffer, pkt);

    GRoom->Handle_LobbyStartGame(session);
}

SendBufferRef Server_PacketHandler::Make_S_MyPlayer(Protocol::ObjectInfo& info)
{
    Protocol::S_MyPlayer pkt;
    *pkt.mutable_info() = info;

    return MakeSendBuffer(pkt, S_MyPlayer);
}

SendBufferRef Server_PacketHandler::Make_S_AddObject(Protocol::S_AddObject& pkt)
{
    return MakeSendBuffer(pkt, S_AddObject);
}

SendBufferRef Server_PacketHandler::Make_S_RemoveObject(Protocol::S_RemoveObject& pkt)
{
    return MakeSendBuffer(pkt, S_RemoveObject);
}

SendBufferRef Server_PacketHandler::Make_S_Move(Protocol::ObjectInfo& info)
{
    Protocol::S_Move pkt;
    *pkt.mutable_info() = info;

    return MakeSendBuffer(pkt, S_Move);
}

SendBufferRef Server_PacketHandler::Make_S_WaveCleared(Protocol::S_WaveCleared& pkt)
{
    return MakeSendBuffer(pkt, S_WaveCleared);
}

SendBufferRef Server_PacketHandler::Make_S_WaveStarted(Protocol::S_WaveStarted& pkt)
{
    return MakeSendBuffer(pkt, S_WaveStarted);
}

SendBufferRef Server_PacketHandler::Make_S_LobbySnapshot(Protocol::S_LobbySnapshot& pkt)
{
    return MakeSendBuffer(pkt, S_LobbySnapshot);
}

SendBufferRef Server_PacketHandler::Make_S_LobbyChat(Protocol::S_LobbyChat& pkt)
{
    return MakeSendBuffer(pkt, S_LobbyChat);
}

SendBufferRef Server_PacketHandler::Make_S_LobbyStartGame()
{
    Protocol::S_LobbyStartGame pkt;

    return MakeSendBuffer(pkt, S_LobbyStartGame);
}
