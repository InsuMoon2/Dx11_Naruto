#include "pch.h"
#include "GameRoom.h"
#include "GameSession.h"
#include "Server_PacketHandler.h"

#include "GameObject.h"
#include "Player.h"
#include "Monster.h"

#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include <nlohmann/json.hpp>

using namespace nlohmann;

NS_BEGIN(Server)
    Shared<GameRoom> GRoom = make_shared<GameRoom>();
NS_END

static bool Try_ReadyObjectType(const json& objJson, Protocol::OBJECT_TYPE& outType)
{
    if (!objJson.contains("object_type"))
        return false;

    if (objJson["object_type"].is_string())
    {
        auto objectType = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
            objJson["object_type"].get<string>());

        if (!objectType.has_value())
            return false;

        outType = objectType.value();
        return true;
    }

    if (objJson["object_type"].is_number_unsigned() || objJson["object_type"].is_number_integer())
    {
        outType = static_cast<Protocol::OBJECT_TYPE>(objJson["object_type"].get<uint32>());
        return true;
    }

    return false;
}

static bool Try_ReadTransform(const json& objJson, Vec3& outPos, float& outYaw)
{
    if (!objJson.contains("components") || !objJson["components"].is_array())
        return false;

    for (const auto& compJson : objJson["components"])
    {
        if (!compJson.contains("type"))
            continue;

        string typeName = compJson["type"].get<string>();
        if (typeName != "COMPONENT_TYPE_TRANSFORM")
            continue;

        if (compJson.contains("position") && compJson["position"].is_array() && compJson["position"].size() >= 3)
        {
            outPos.x = compJson["position"][0].get<float>();
            outPos.y = compJson["position"][1].get<float>();
            outPos.z = compJson["position"][2].get<float>();
        }

        if (compJson.contains("rotation") && compJson["rotation"].is_array() && compJson["rotation"].size() >= 2)
        {
            outYaw = compJson["rotation"][1].get<float>();
        }

        return true;
    }

    return false;
}

void GameRoom::Enter_GameRoom(Shared<GameSession> session, const Protocol::C_EnterGame& pkt)
{
    Ensure_LevelMonstersSpawned();

    auto player = Player::Create();
    player->Set_Session(session);
    session->Set_PlayerId(player->Get_ObjectID());

    //player->info = pkt.info();

    for (auto& pair : pkt.info().equipparts())
    {
        (*player->info.mutable_equipparts())[pair.first] = pair.second;
    }

    auto* protoPos = player->info.mutable_pos();
    protoPos->set_x(pkt.spawn_pos().x());
    protoPos->set_y(pkt.spawn_pos().y());
    protoPos->set_z(pkt.spawn_pos().z());
    player->info.set_rot_y(pkt.rot_y());

    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_MyPlayer(player->info);
        session->Send(sendBuffer);
    }

    {
        Protocol::S_AddObject pkt;

        for (auto& [id, existingPlayer] : _players)
        {
            Protocol::ObjectInfo* info = pkt.add_objects();
            *info = existingPlayer->info;
        }

        for (auto& [id, existingMonster] : _monsters)
        {
            Protocol::ObjectInfo* info = pkt.add_objects();
            *info = existingMonster->info;
        }

        if (pkt.objects_size() > 0)
        {
            SendBufferRef sendBuffer = Server_PacketHandler::Make_S_AddObject(pkt);
            session->Send(sendBuffer);
        }
    }

    Add_Player(player);
}

void GameRoom::Leave_GameRoom(Shared<GameSession> session)
{
    if (session == nullptr)
        return;

    Shared<Player> player = Find_Player(session->Get_PlayerId());
    if (player == nullptr)
        return;

    uint64 id = player->Get_ObjectID();
    Remove_Player(id);

    cout << "[Room] Player " << id
        << " Left (" << _players.size() << " players)" << endl;

}

uint64 GameRoom::Get_MonsterAuthorityPlayerId() const
{
    if (_players.empty())
        return 0;

    return _players.begin()->first;
}

void GameRoom::Handle_C_Move(Shared<GameSession> session, Protocol::C_Move& pkt)
{
    CHECK_NULL(session);

    uint64 id = pkt.info().objectid();

    if (pkt.info().objecttype() == Protocol::OBJECT_TYPE_MONSTER ||
        pkt.info().objecttype() == Protocol::OBJECT_TYPE_BOSS_PAIN)
    {
        const uint64 authorityPlayerId = Get_MonsterAuthorityPlayerId();
        if (authorityPlayerId == 0 || session->Get_PlayerId() != authorityPlayerId)
            return;

        Shared<Monster> monster = Find_Monster(id);
        if (monster == nullptr)
            return;

        const Protocol::OBJECT_TYPE networkObjectType = monster->info.objecttype();
        monster->info = pkt.info();
        monster->info.set_objectid(monster->Get_ObjectID());
        monster->info.set_objecttype(networkObjectType);

        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(monster->info);
        Broadcast(sendBuffer);
        return;
    }

    Shared<Player> player = Find_Player(id);
    if (player == nullptr)
        return;

    // 서버 측 위치 갱신
    player->info = pkt.info();
    player->info.set_objectid(id);

    // 모든 플레이어들에게 이동 패킷 전달
    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(player->info);
        Broadcast(sendBuffer);
    }

}

void GameRoom::Broadcast(SendBufferRef sendBuffer)
{
    for (auto& [id, player] : _players)
    {
        if (player->Get_Session())
            player->Get_Session()->Send(sendBuffer);
    }
}

void GameRoom::Update(float timeDelta)
{
    for (auto& [id, player] : _players)
    {
        player->Update();
    }

    // [변경] 몬스터는 권한 클라이언트가 기존 BT를 그대로 돌리고, 서버는 릴레이만 담당한다.
}

void GameRoom::Ensure_LevelMonstersSpawned()
{
    if (_levelMonstersSpawned)
       return;

    vector<FServerMonsterSpawnDesc> spawnDescs;
    if (!Load_MonsterSpawnData_FromLevel(L"[20260420]Tutorial", spawnDescs))
        return;

    for (const auto& spawnDesc : spawnDescs)
    {
        auto monster = Monster::Create();
        CHECK_NULL(monster);

        monster->Initialize_FromSpawn(spawnDesc.position, spawnDesc.yaw);
        monster->info.set_objecttype(spawnDesc.objectType);

        Add_Monster(monster);
    }

    _levelMonstersSpawned = true;
}

bool GameRoom::Load_MonsterSpawnData_FromLevel(const wstring& levelName, vector<FServerMonsterSpawnDesc>& outSpawns)
{
    const wstring fullPath =
        L"../../../Client/Bin/Resources/Data/json/Levels/" + levelName + L".level.json";

     ifstream file(fullPath);
    if (!file.is_open())
        return false;

    json levelJson;
    file >> levelJson;

    if (!levelJson.contains("gameObjects") || !levelJson["gameObjects"].is_array())
        return false;

    for (const auto& objJson : levelJson["gameObjects"])
    {
        Protocol::OBJECT_TYPE objectType = Protocol::OBJECT_TYPE_NONE;
        if (!Try_ReadyObjectType(objJson, objectType))
            continue;

        if (objectType != Protocol::OBJECT_TYPE_MONSTER &&
            objectType != Protocol::OBJECT_TYPE_BOSS_PAIN)
            continue;

        FServerMonsterSpawnDesc desc{};
        desc.objectType = objectType;

        if (objJson.contains("prefab_name") && objJson["prefab_name"].is_string())
            desc.prefabName = objJson["prefab_name"].get<string>();

        if (objJson.contains("layerTag") && objJson["layerTag"].is_string())
        {
            const string layerTag = objJson["layerTag"].get<string>();
            desc.layerTag = wstring(layerTag.begin(), layerTag.end());
        }
            
        Try_ReadTransform(objJson, desc.position, desc.yaw);

        outSpawns.push_back(desc);
    }

    return true;
}

