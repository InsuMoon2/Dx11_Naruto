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

    default:
        break;
    }

}

void Server_PacketHandler::Handle_C_Move(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    Protocol::C_Move pkt;
    ParsePacket(buffer, pkt);

    // objectId를 세션의 playerId로 설정 -> 클라 조작 방지용
    pkt.mutable_info()->set_objectid(session->Get_PlayerId());

    GRoom->Handle_C_Move(pkt);
}

void Server_PacketHandler::Handle_C_EnterGame(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    if (session->Get_PlayerId() != 0)
        return;

    Protocol::C_EnterGame pkt;
    ParsePacket(buffer, pkt);

    GRoom->Enter_GameRoom(session, pkt);
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
