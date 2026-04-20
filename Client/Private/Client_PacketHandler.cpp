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
#include "EnemyCharacter.h"

#include <algorithm>
#include <limits>

static uint64 s_MyNetworkId = 0;

// 네트워크로 생성된 플레이어/몬스터를 objectId 기준으로 추적해보기
static umap<uint64, Weak<GameObject>> s_NetworkObjects;

static bool Is_PlayerNetworkObject(const Shared<GameObject>& gameObject)
{
    return gameObject && dynamic_pointer_cast<Player>(gameObject) != nullptr;
}

static bool Is_CurrentClientMonsterAuthority()
{
    if (s_MyNetworkId == 0)
        return false;

    uint64 authorityPlayerId = (std::numeric_limits<uint64>::max)();

    for (const auto& [objectId, weakObject] : s_NetworkObjects)
    {
        auto gameObject = weakObject.lock();
        if (!Is_PlayerNetworkObject(gameObject))
            continue;

        authorityPlayerId = (std::min)(authorityPlayerId, objectId);
    }

    return authorityPlayerId != (std::numeric_limits<uint64>::max)() && authorityPlayerId == s_MyNetworkId;
}

static void Refresh_MonsterAuthority()
{
    const bool isAuthority = Is_CurrentClientMonsterAuthority();

    for (auto& [objectId, weakObject] : s_NetworkObjects)
    {
        auto gameObject = weakObject.lock();
        auto enemy = dynamic_pointer_cast<EnemyCharacter>(gameObject);
        if (!enemy)
            continue;

        enemy->Set_NetworkDriven(!isAuthority);
        enemy->Set_Local(isAuthority);
    }
}

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

    auto existingIt = s_NetworkObjects.find(myId);
    if (existingIt != s_NetworkObjects.end())
    {
        auto existing = existingIt->second.lock();
        if (existing)
        {
            Refresh_MonsterAuthority();
            return;
        }
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

    s_NetworkObjects[myId] = player;
    Refresh_MonsterAuthority();

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

        // 내 플레이어는 스킵
        if (objectId == s_MyNetworkId)
            continue;

        // 이미 같은 objectId를 가진 네트워크 오브젝트가 살아있으면 중복생성 ㄴㄴ
        auto it = s_NetworkObjects.find(objectId);
        if (it != s_NetworkObjects.end())
        {
            if (!it->second.expired())
                continue;
        }

        uint32 levelIndex = GAME->Current_Level();

        auto gameObject = Spawn_NetworkObject(info, levelIndex);
        if (!gameObject)
            continue;

        // 원격 플레이어면 장비 파츠, 네트워크 ID 세팅
        if (auto remotePlayer = dynamic_pointer_cast<Player>(gameObject))
        {
            for (auto& pair : info.equipparts())
            {
                ContainerObject::EPartSlot slot = static_cast<ContainerObject::EPartSlot>(pair.first);
                wstring assetTag = wstring(pair.second.begin(), pair.second.end());
                remotePlayer->Apply_CustomizingPart(slot, assetTag);
            }

            remotePlayer->Set_Local(false);
            remotePlayer->Set_NetworkId(objectId);
        }

        // 네트워크 몬스터면, 로컬 AI를 끄고 서버 상태에 따라서 움직이도록 -> Behavior Tree 동기화 어떻게 할지?
        auto enemy = dynamic_pointer_cast<EnemyCharacter>(gameObject);
        if (enemy)
        {
            enemy->Set_Local(false);
            enemy->Set_NetworkDriven(true);
            enemy->Set_NetworkObjectId(objectId);
        }

        // 패킷의 위치/회전/상태를 실제 오브젝트에 반영
        Apply_NetworkObjectInfo(gameObject, info);

        // 이후 S_Move / S_RemoveObject에서 찾을 수 있게 저장한다.
        s_NetworkObjects[objectId] = gameObject;
    }

    Refresh_MonsterAuthority();
}


void Client_PacketHandler::Handle_S_RemoveObject(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_RemoveObject pkt;
    ParsePacket(buffer, pkt);

    for (int i = 0; i < pkt.ids_size(); i++)
    {
        int64 id = pkt.ids(i);

        auto it = s_NetworkObjects.find(id);
        if (it == s_NetworkObjects.end())
            continue;

        auto gameObject = it->second.lock();
        if (gameObject)
        {
            EVENT->Publish(FEvent_Object::Create(EEventType::Delete_Object, gameObject));
        }

        s_NetworkObjects.erase(it);
    }

    Refresh_MonsterAuthority();
}

void Client_PacketHandler::Handle_S_Move(Shared<ServerSession> session, BYTE* buffer, int32 len)
{
    Protocol::S_Move pkt;
    ParsePacket(buffer, pkt);

    uint64 objectId = pkt.info().objectid();

    if (objectId == s_MyNetworkId)
        return;

    auto it = s_NetworkObjects.find(objectId);
    if (it == s_NetworkObjects.end())
        return;

    auto gameObject = it->second.lock();
    if (!gameObject)
    {
        s_NetworkObjects.erase(it);
        return;
    }

    Apply_NetworkObjectInfo(gameObject, pkt.info());
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

Shared<GameObject> Client_PacketHandler::Spawn_NetworkObject(const Protocol::ObjectInfo& info, uint32 levelIndex)
{
    switch (info.objecttype())
    {
    case Protocol::OBJECT_TYPE_PLAYER:
        return Spawn_Helper::Prefab("RemotePlayer")
            .AtLevel(levelIndex)
            .InLayer(TEXT("Layer_GameObject"))
            .Spawn();

    case Protocol::OBJECT_TYPE_MONSTER:
        return Spawn_Helper::Prefab("Monster")
            .AtLevel(levelIndex)
            .InLayer(TEXT("Layer_GameObject"))
            .Spawn();

    case Protocol::OBJECT_TYPE_BOSS_PAIN:
        return Spawn_Helper::Prefab("Boss")
            .AtLevel(levelIndex)
            .InLayer(TEXT("Layer_GameObject"))
            .Spawn();

    default:
        return nullptr;
    }
}

void Client_PacketHandler::Apply_NetworkObjectInfo(Shared<GameObject> gameObject, const Protocol::ObjectInfo& info)
{
    CHECK_NULL(gameObject);

    if (auto player = dynamic_pointer_cast<Player>(gameObject))
    {
        player->Sync(info);
        return;
    }

    if (auto enemy = dynamic_pointer_cast<EnemyCharacter>(gameObject))
    {
        if (!enemy->Is_NetworkDriven())
            return;

        enemy->Sync(info);
        return;
    }
}
