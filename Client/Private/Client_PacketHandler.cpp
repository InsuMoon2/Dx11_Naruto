#include "pch.h"
#include "Client_PacketHandler.h"
#include "BufferReader.h"
#include "Level_Manager.h"
#include "Event_Manager.h"
#include "Player.h"
#include "MyPlayer.h"
#include "RemotePlayer.h"

static uint64 s_MyNetworkId = 0;
static umap<uint64, Weak<Player>> s_NetworkPlayers;

void Client_PacketHandler::HandlePacket(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    BufferReader br(buffer, len);

    PacketHeader header;
    br >> header;

    switch (header.id)
    {
    case S_TEST:
        Handle_S_TEST(session, buffer, len);
        break;
    case S_MyPlayer:
        Handle_S_MyPlayer(session, buffer, len);
        break;
    case S_AddObject:
        Handle_S_AddObject(session, buffer, len);
        break;
    case S_RemoveObject:
        Handle_S_RemoveObject(session, buffer, len);
        break;

    case S_Move:
        Handle_S_Move(session, buffer, len);
        break;
    }


}

void Client_PacketHandler::Handle_S_TEST(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_TEST pkt;
    ParsePacket(buffer, pkt);

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

void Client_PacketHandler::Handle_S_MyPlayer(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_MyPlayer pkt;
    ParsePacket(buffer, pkt);

    uint64 myId = pkt.info().objectid();
    s_MyNetworkId = myId;

    // MyPlayer 스폰
    uint32 levelIndex = GAME->Current_Level();

    auto gameObject = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_PLAYER,   
        levelIndex,
        TEXT("Layer_GameObject"));

    if (!gameObject)
        return;

    auto player = dynamic_pointer_cast<Player>(gameObject);
    if (player)
    {
        player->Set_NetworkId(myId);
        s_NetworkPlayers[myId] = player;

        // PlayerStart 위치 찾기
        auto gameObjects = GAME->Get_GameObjects(GAME->Current_Level());
        for (auto& obj : gameObjects)
        {
            if (obj->Get_ObjectType() == Protocol::OBJECT_TYPE_PLAYER_START)
            {
                player->Get_Component<Transform>()->Set_LocalPosition(
                    obj->Get_Component<Transform>()->Get_WorldPosition());
                break;
            }
        }

        GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(
            player->Get_Component<Transform>());
    }

}

void Client_PacketHandler::Handle_S_AddObject(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_AddObject pkt;
    ParsePacket(buffer, pkt);

    // 오브젝트 순회
    for (int i = 0; i < pkt.objects_size(); i++)
    {
        const Protocol::ObjectInfo& info = pkt.objects(i);
        uint64 objectId = info.objectid();

        // 자기 아이디는 무시
        if (objectId == s_MyNetworkId)
            continue;

        // 등록 된 플레이언지 확인
        auto it = s_NetworkPlayers.find(objectId);
        if (it != s_NetworkPlayers.end())
        {
            // 아직 존재하면, 스킵
            if (!it->second.expired())
                continue;
        }

        // RemotePlayer 스폰
        uint32 levelIndex = GAME->Current_Level();

        auto gameObject = GAME->Clone_And_Add_GameObject(
            ETOI(ELevelType::Static),
            Protocol::OBJECT_TYPE_REMOTE_PLAYER,
            levelIndex,
            TEXT("Layer_GameObject"));

        if (!gameObject)
            continue;

        // NetworkId 세팅, map에 등록
        auto remote = dynamic_pointer_cast<Player>(gameObject);
        if (remote)
        {
            remote->Set_NetworkId(objectId);
            s_NetworkPlayers[objectId] = remote;
        }

    }
}

void Client_PacketHandler::Handle_S_RemoveObject(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_RemoveObject pkt;
    ParsePacket(buffer, pkt);

    // 삭제 대상 순회
    for (int i = 0; i < pkt.ids_size(); i++)
    {
        int64 id = pkt.ids(i);

        auto it = s_NetworkPlayers.find(id);
        if (it == s_NetworkPlayers.end())
            continue;

        auto player = it->second.lock();
        if (player)
        {
            EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, player));
        }

        s_NetworkPlayers.erase(it);
    }
}

void Client_PacketHandler::Handle_S_Move(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_Move pkt;
    ParsePacket(buffer, pkt);

    uint64 objectId = pkt.info().objectid();
    float x = pkt.info().pos().x();
    float y = pkt.info().pos().y();
    float z = pkt.info().pos().z();
    float rotY = pkt.info().rot_y();

    // 내 캐릭터 패킷이라면 무시
    if (objectId == s_MyNetworkId)
        return;

    // 해당 objectId의 RemotePlayer 찾기
    auto it = s_NetworkPlayers.find(objectId);
    if (it == s_NetworkPlayers.end())
        return;     // 모르는 ID -> 무시

    auto player = it->second.lock();
    if (!player)
    {
        // 이미 파괴된 오브젝트
        s_NetworkPlayers.erase(it);
        return;
    }

    // Snyc -> RemotePlayer에서 보간처리
    player->Sync(pkt.info());
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
