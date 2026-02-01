#include "pch.h"
#include "Server_PacketHandler.h"
#include "BufferReader.h"
#include "BufferWriter.h"
#include "GameSession.h"

void Server_PacketHandler::HandlePacket(shared_ptr<GameSession> session, BYTE* buffer, int32 len)
{
    BufferReader reader(buffer, len);

    PacketHeader header;
    reader.Peek(&header);

    switch (header.id)
    {


    default:
        break;
    }

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

    return MakeSendBuffer(pkt, S_TEST);
}
