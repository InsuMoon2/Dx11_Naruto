#include "pch.h"
#include "Client_PacketHandler.h"
#include "BufferReader.h"
#include "Level_Manager.h"
#include "Event_Manager.h"
#include "Player.h"
#include "MyPlayer.h"
#include "RemotePlayer.h"
#include "Spawn_Helper.h"
#include "Customizer_Manager.h"

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

    auto existingIt = s_NetworkPlayers.find(myId);
    if (existingIt != s_NetworkPlayers.end())
    {
        auto existing = existingIt->second.lock();
        if (existing)
            return;
    }

    uint32 levelIndex = GAME->Current_Level();

    auto gameObject = Spawn_Helper::Prefab("TestPlayer2")
        .AtLevel(levelIndex)
        .InLayer(TEXT("Layer_Player"))
        .Spawn();

    if (!gameObject)
        return;

    auto player = dynamic_pointer_cast<Player>(gameObject);
    if (!player)
        return;

    for (auto& pair : pkt.info().equipparts())
    {
        ContainerObject::EPartSlot slot = static_cast<ContainerObject::EPartSlot>(pair.first);
        wstring assetTag = Utils::ToWString(pair.second);

        player->Apply_CustomizingPart(slot, assetTag);

    }

    player->Set_NetworkId(myId);
    player->Sync(pkt.info());

    s_NetworkPlayers[myId] = player;

    GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(player->Get_Component<Transform>());
    GAME->Get_DelegateHub().OnPlayerObjectSpawned.Broadcast(player);
}

void Client_PacketHandler::Handle_S_AddObject(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_AddObject pkt;
    ParsePacket(buffer, pkt);

    for (int i = 0; i < pkt.objects_size(); i++)
    {
        const Protocol::ObjectInfo& info = pkt.objects(i);
        uint64 objectId = info.objectid();

        if (objectId == s_MyNetworkId)
            continue;

        auto it = s_NetworkPlayers.find(objectId);
        if (it != s_NetworkPlayers.end())
        {
            if (!it->second.expired())
                continue;
        }

        uint32 levelIndex = GAME->Current_Level();

        auto gameObject = Spawn_Helper::Prefab("RemotePlayer")
            .AtLevel(levelIndex)
            .InLayer(TEXT("Layer_GameObject"))
            .Spawn();

        if (!gameObject)
            continue;

        auto remote = dynamic_pointer_cast<Player>(gameObject);
        if (!remote)
            continue;

        for (auto& pair : info.equipparts())
        {
            ContainerObject::EPartSlot slot = static_cast<ContainerObject::EPartSlot>(pair.first);
            wstring assetTag = Utils::ToWString(pair.second);

            remote->Apply_CustomizingPart(slot, assetTag);
        }

        remote->Set_NetworkId(objectId);
        remote->Set_Local(false);
        remote->Sync(info);

        s_NetworkPlayers[objectId] = remote;
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

SendBufferRef Client_PacketHandler::Make_C_Move(const Protocol::ObjectInfo& objectInfo)
{
    Protocol::C_Move pkt;

    *pkt.mutable_info() = objectInfo;

    return MakeSendBuffer(pkt, C_Move);
}

SendBufferRef Client_PacketHandler::Make_C_EnterGame(const Vec3& spawnPos, float rotY)
{
    Protocol::C_EnterGame pkt;

    auto* protoPos = pkt.mutable_spawn_pos();
    protoPos->set_x(spawnPos.x);
    protoPos->set_y(spawnPos.y);
    protoPos->set_z(spawnPos.z);

    pkt.set_rot_y(rotY);

    auto custom = GET_SINGLE(Customizer_Manager);
    const auto& customDesc = custom->Get_CustomizerDesc();

    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        const wstring& partTag = customDesc.partTags[i];
        if (!partTag.empty())
        {
            (*pkt.mutable_info()->mutable_equipparts())[i] = Utils::ToString(partTag);
        }
    }

    return MakeSendBuffer(pkt, C_EnterGame);
}
