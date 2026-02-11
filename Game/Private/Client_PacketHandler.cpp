#include "pch.h"
#include "Client_PacketHandler.h"
#include "BufferReader.h"
#include "Level_Manager.h"

void Client_PacketHandler::HandlePacket(shared_ptr<ServerSession> session, BYTE* buffer, int32 len)
{
    BufferReader br(buffer, len);

    PacketHeader header;
    br >> header;

    switch (header.id)
    {
    case S_TEST:
        Handle_S_TEST(session, buffer, len);
        break;

    default:
        break;

    }
}

void Client_PacketHandler::Handle_S_TEST(shared_ptr<ServerSession> session, BYTE* buffer, int32 len)
{
    PacketHeader* header = (PacketHeader*)(buffer);
    //uint16 Id = header->id;
    uint16 size = header->size;

    Protocol::S_TEST pkt;
    pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

    uint64 id = pkt.id();
    uint64 hp = pkt.hp();
    uint16 attack = pkt.attack();

    LOG_INFO("Recv S_TEST | ID: {}, HP: {}, ATT: {}", id, hp, attack);

    for (int32 i = 0; i < pkt.buffs_size(); i++)
    {
        const Protocol::BuffData& data = pkt.buffs(i);
        LOG_INFO("  - BuffInfo: ID {}, Time {:.2f}", data.buffid(), data.remaintime());
    }
}

SendBufferRef Client_PacketHandler::Make_C_Move()
{
    //Protocol::C_MOVE pkt;
    //pkt.set_objectid(1234); // 테스트 ID
    //pkt.set_posx(10.5f);
    //pkt.set_posy(20.5f);
    //pkt.set_posz(0.0f);
    //return MakeSendBuffer(pkt, C_MOVE);

    return nullptr;
}
