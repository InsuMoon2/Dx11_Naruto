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

    case S_Move:
        Handle_S_Move(session, buffer, len);
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

void Client_PacketHandler::Handle_S_Move(shared_ptr<ServerSession> session, BYTE* buffer, int32 len)
{
    PacketHeader* header = (PacketHeader*)buffer;
    uint16 size = header->size;

    Protocol::S_Move pkt;
    pkt.ParseFromArray(&header[1], size - sizeof(PacketHeader));

    uint64 objectId = pkt.info().objectid();
    float x = pkt.info().pos().x();
    float y = pkt.info().pos().y();
    float z = pkt.info().pos().z();
    float rotY = pkt.info().rot_y();

    // TODO : ObjectManager에서 objefctId로 GameObject를 찾아서 위치 갱신
    // 자기 자신이면 무시?
}

SendBufferRef Client_PacketHandler::Make_C_Move(float x, float y, float z, float rotY)
{
    Protocol::C_Move pkt;
    auto* info = pkt.mutable_info();
    auto* pos = info->mutable_pos();
    pos->set_x(x);
    pos->set_y(y);
    pos->set_z(z);
    info->set_rot_y(rotY);

    return MakeSendBuffer(pkt, C_Move);
}
