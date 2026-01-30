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
    return nullptr;
}
