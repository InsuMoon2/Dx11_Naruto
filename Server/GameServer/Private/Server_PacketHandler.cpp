#include "pch.h"
#include "Server_PacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"
#include "GameSessionManager.h"

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

    default:
        break;
    }

}

void Server_PacketHandler::Handle_C_Move(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    PacketHeader* header = (PacketHeader*)buffer;

    Protocol::C_Move pkt;
    pkt.ParseFromArray(&header[1], header->size - sizeof(PacketHeader));

    // S_Move로 전체 브로드캐스트 - Make 함수 사용이라는데
    GSessionManager.Broadcast(Make_S_Move(pkt, session->Get_PlayerId()));
}

SendBufferRef Server_PacketHandler::Make_S_TEST(uint64 id, uint32 hp, uint16 attack, vector<BuffData> buffs)
{
    Protocol::S_TEST pkt;

    pkt.set_id(10);
    pkt.set_hp(100);
    pkt.set_attack(10);

    {
        Protocol::BuffData* data = pkt.add_buffs();
        data->set_buffid(100);
        data->set_remaintime(1.2f);
        {
            data->add_victims(10);
        }
    }
    {
        Protocol::BuffData* data = pkt.add_buffs();
        data->set_buffid(200);
        data->set_remaintime(2.2f);
        {
            data->add_victims(20);
        }
    }

    return MakeSendBuffer(pkt, S_Test);
}

SendBufferRef Server_PacketHandler::Make_S_MyPlayer(uint64 playerId)
{
    Protocol::S_MyPlayer pkt;
    pkt.mutable_info()->set_objectid(playerId);
    pkt.mutable_info()->set_name("Player_" + ::to_string(playerId));

    return MakeSendBuffer(pkt, S_MyPlayer);
}

SendBufferRef Server_PacketHandler::Make_S_AddObject(uint64 playerId)
{
    Protocol::S_AddObject pkt;
    auto* info = pkt.add_objects();

    info->set_objectid(playerId);
    info->set_name("Player_" + to_string(playerId));

    return MakeSendBuffer(pkt, S_AddObject);
}

SendBufferRef Server_PacketHandler::Make_S_RemoveObject(uint64 playerId)
{
    Protocol::S_RemoveObject pkt;
    pkt.add_ids(playerId);

    return MakeSendBuffer(pkt, S_RemoveObject);
}

SendBufferRef Server_PacketHandler::Make_S_Move(Protocol::C_Move& recvPkt, uint64 playerId)
{
    Protocol::S_Move movePkt;
    *movePkt.mutable_info() = recvPkt.info();

    movePkt.mutable_info()->set_objectid(playerId);

    return MakeSendBuffer(movePkt, S_Move);
}
