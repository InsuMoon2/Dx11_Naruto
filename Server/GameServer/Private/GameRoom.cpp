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

static Vec3 Json_ToVec3(const json& value, const Vec3& fallback = Vec3(0.f, 0.f, 0.f))
{
    if (!value.is_array() || value.size() < 3)
        return fallback;

    return Vec3(
        value[0].get<float>(),
        value[1].get<float>(),
        value[2].get<float>());
}

static bool Try_ReadTriggerExtents(const json& objJson, Vec3& outExtents)
{
    if (!objJson.contains("components") || !objJson["components"].is_array())
        return false;

    for (const auto& compJson : objJson["components"])
    {
        if (!compJson.contains("type"))
            continue;

        if (!compJson["type"].is_string())
            continue;

        // Collider::To_Json()은 콜라이더 종류를 "COMPONENT_TYPE_COLLIDER"로, 세부 도형은 "shape"로 따로 적는다.
        if (compJson["type"].get<string>() != "COMPONENT_TYPE_COLLIDER")
            continue;

        if (!compJson.contains("shape") || compJson["shape"].get<string>() != "OBB")
            continue;

        // extents는 최상위가 아니라 "bounding" 하위에 중첩되어 저장된다.
        if (compJson.contains("bounding") && compJson["bounding"].contains("extents") &&
            compJson["bounding"]["extents"].is_array() && compJson["bounding"]["extents"].size() >= 3)
        {
            outExtents = Json_ToVec3(compJson["bounding"]["extents"], Vec3(2.f, 2.f, 2.f));
            return true;
        }
    }

    return false;
}

static Vec3 Rotate_AroundY(const Vec3& value, float yawDegree)
{
    constexpr float DegToRad = 3.14159265358979323846f / 180.f;

    const float radian = yawDegree * DegToRad;
    const float cosValue = cosf(radian);
    const float sinValue = sinf(radian);

    return Vec3(
        value.x * cosValue + value.z * sinValue,
        value.y,
        -value.x * sinValue + value.z * cosValue);
}

static Vec3 Resolve_WaveTriggerWorldPosition(const Vec3& triggerPosition, float triggerYaw, const Vec3& localPosition)
{
    return triggerPosition + Rotate_AroundY(localPosition, triggerYaw);
}

static float Resolve_WaveTriggerWorldYaw(float triggerYaw, const Vec3& localRotation)
{
    return triggerYaw + localRotation.y;
}

static bool IsPointInsideWaveTrigger(const Vec3& point, const GameRoom::FServerWaveTriggerDesc& trigger)
{
    const Vec3 delta = point - trigger.position;
    const Vec3 localPoint = Rotate_AroundY(delta, -trigger.yaw);

    return fabsf(localPoint.x) <= trigger.extents.x &&
           fabsf(localPoint.y) <= trigger.extents.y &&
           fabsf(localPoint.z) <= trigger.extents.z;
}


void GameRoom::Enter_GameRoom(Shared<GameSession> session, const Protocol::C_EnterGame& pkt)
{
    Ensure_LevelMonstersSpawned();
    Ensure_LevelWaveTriggersLoaded();

    Shared<Player> player = nullptr;
    const uint64 existingPlayerId = session->Get_PlayerId();

    if (existingPlayerId != 0)
    {
        player = Find_Player(existingPlayerId);
    }

    const bool isReenter = (player != nullptr);

    if (!player)
    {
        player = Player::Create();
        session->Set_PlayerId(player->Get_ObjectID());
    }

    player->Set_Session(session);

    if (!pkt.info().name().empty())
    {
        player->info.set_name(pkt.info().name());
    }

    player->info.mutable_equipparts()->clear();
    for (auto& pair : pkt.info().equipparts())
    {
        (*player->info.mutable_equipparts())[pair.first] = pair.second;
    }
    player->info.set_weapon_type(pkt.info().weapon_type());

    auto* protoPos = player->info.mutable_pos();

    // 접속한 순서(현재 방에 있는 플레이어 수)에 따라 X축 위치를 5씩 띄워서 겹치지 않게 스폰합니다.
    float spawnOffsetX = isReenter ? 0.f : static_cast<float>(_players.size()) * 5.f;

    protoPos->set_x(pkt.spawn_pos().x() + spawnOffsetX);
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

    if (!isReenter)
    {
        Add_Player(player);
        return;
    }

    SendBufferRef moveBuffer = Server_PacketHandler::Make_S_Move(player->info);
    Broadcast(moveBuffer);
}

void GameRoom::Leave_GameRoom(Shared<GameSession> session)
{
    if (session == nullptr)
        return;

    bool lobbyChanged = false;
    for (auto iter = _lobbyPlayers.begin(); iter != _lobbyPlayers.end(); )
    {
        auto lobbySession = iter->second.session.lock();
        if (lobbySession == session || !lobbySession)
        {
            iter = _lobbyPlayers.erase(iter);
            lobbyChanged = true;
            continue;
        }

        ++iter;
    }

    if (lobbyChanged)
        Broadcast_LobbySnapshot();

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

    Shared<Monster> monster = Find_Monster(id);
    if (monster != nullptr)
    {
        const uint64 authorityPlayerId = Get_MonsterAuthorityPlayerId();
        if (authorityPlayerId == 0 || session->Get_PlayerId() != authorityPlayerId)
            return;

        const Protocol::OBJECT_TYPE networkObjectType = monster->info.objecttype();
        const string prefabName = monster->info.name();
        monster->info = pkt.info();
        monster->info.set_objectid(monster->Get_ObjectID());
        monster->info.set_objecttype(networkObjectType);
        if (!prefabName.empty())
            monster->info.set_name(prefabName);

        const bool isDeadByState =
            monster->info.object_state() == Protocol::OBJECT_STATE_TYPE_DEAD;
        const bool isDeadByHp =
            monster->info.has_stat() && monster->info.stat().current_hp() <= 0.f;

        if (isDeadByState || isDeadByHp)
        {
            Remove_Monster(monster->Get_ObjectID());
            return;
        }

        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(monster->info);
        Broadcast(sendBuffer);
        return;
    }

    Shared<Player> player = Find_Player(id);
    if (player == nullptr)
        return;

    Protocol::ObjectInfo& currentInfo = player->info;
    const Protocol::ObjectInfo& nextInfo = pkt.info();

    currentInfo.set_objectid(id);
    currentInfo.set_objecttype(Protocol::OBJECT_TYPE_PLAYER);
    currentInfo.mutable_pos()->CopyFrom(nextInfo.pos());
    currentInfo.set_rot_x(nextInfo.rot_x());
    currentInfo.set_rot_y(nextInfo.rot_y());
    currentInfo.set_rot_z(nextInfo.rot_z());
    currentInfo.set_object_state(nextInfo.object_state());
    currentInfo.set_move_dir(nextInfo.move_dir());
    currentInfo.set_anim_phase(nextInfo.anim_phase());
    currentInfo.set_anim_force_restart(nextInfo.anim_force_restart());
    currentInfo.set_attack_profile(nextInfo.attack_profile());
    currentInfo.set_attack_combo_index(nextInfo.attack_combo_index());
    currentInfo.set_anim_state_key(nextInfo.anim_state_key());
    currentInfo.set_hit_reaction_type(nextInfo.hit_reaction_type());
    currentInfo.set_hit_reaction_serial(nextInfo.hit_reaction_serial());
    currentInfo.set_weapon_type(nextInfo.weapon_type());

    if (nextInfo.has_stat())
        currentInfo.mutable_stat()->CopyFrom(nextInfo.stat());
    else
        currentInfo.clear_stat();

    if (!nextInfo.name().empty())
        currentInfo.set_name(nextInfo.name());

    // 모든 플레이어들에게 이동 패킷 전달
    {
        SendBufferRef sendBuffer = Server_PacketHandler::Make_S_Move(currentInfo);
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

void GameRoom::Broadcast_Lobby(SendBufferRef sendBuffer)
{
    Prune_LobbyPlayers();

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto session = lobbyPlayer.session.lock();
        if (!session)
            continue;

        session->Send(sendBuffer);
    }
}

void GameRoom::Update(float timeDelta)
{
    for (auto& [id, player] : _players)
    {
        player->Update();
    }

    Update_WaveTriggers();
}

void GameRoom::Join_Lobby(Shared<GameSession> session, const Protocol::C_LobbyJoin& pkt)
{
    CHECK_NULL(session);

    Prune_LobbyPlayers();

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession == session)
        {
            lobbyPlayer.info = pkt.info();
            cout << "[Lobby] Refresh lobbyId=" << lobbyId
                << " slot=" << lobbyPlayer.slot
                << " host=" << (lobbyPlayer.slot == 1 ? "true" : "false")
                << " players=" << _lobbyPlayers.size() << endl;
            Broadcast_LobbySnapshot();
            return;
        }
    }

    if (_lobbyPlayers.size() >= 2)
    {
        cout << "[Lobby] Join rejected: lobby full" << endl;
        return;
    }

    FLobbyPlayer lobbyPlayer{};
    lobbyPlayer.lobbyId = _nextLobbyId++;
    lobbyPlayer.slot = static_cast<uint32>(_lobbyPlayers.size()) + 1;
    lobbyPlayer.info = pkt.info();
    lobbyPlayer.session = session;

    _lobbyPlayers[lobbyPlayer.lobbyId] = lobbyPlayer;
    cout << "[Lobby] Join lobbyId=" << lobbyPlayer.lobbyId
        << " slot=" << lobbyPlayer.slot
        << " host=" << (lobbyPlayer.slot == 1 ? "true" : "false")
        << " players=" << _lobbyPlayers.size() << endl;

    Broadcast_LobbySnapshot();
}

void GameRoom::Handle_LobbyChat(Shared<GameSession> session, const Protocol::C_LobbyChat& pkt)
{
    CHECK_NULL(session);

    if (pkt.message().empty())
        return;

    string message = pkt.message();
    if (message.size() > 80)
        message = message.substr(0, 80);

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession != session)
            continue;

        Protocol::S_LobbyChat chatPkt;
        chatPkt.set_lobby_id(lobbyId);
        chatPkt.set_name(lobbyPlayer.info.name().empty() ? "Player" : lobbyPlayer.info.name());
        chatPkt.set_message(message);

        Broadcast_Lobby(Server_PacketHandler::Make_S_LobbyChat(chatPkt));
        return;
    }
}

void GameRoom::Handle_LobbyStartGame(Shared<GameSession> session)
{
    CHECK_NULL(session);

    Prune_LobbyPlayers();

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto existingSession = lobbyPlayer.session.lock();
        if (existingSession != session)
            continue;

        if (lobbyPlayer.slot != 1)
            return;

        Broadcast_Lobby(Server_PacketHandler::Make_S_LobbyStartGame());
        return;
    }
}

void GameRoom::Send_LobbySnapshot(Shared<GameSession> session)
{
    CHECK_NULL(session);

    Prune_LobbyPlayers();

    uint64 myLobbyId = 0;
    Protocol::S_LobbySnapshot pkt;

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto lobbySession = lobbyPlayer.session.lock();
        if (lobbySession == session)
            myLobbyId = lobbyId;

        auto* info = pkt.add_players();
        info->set_lobby_id(lobbyPlayer.lobbyId);
        info->set_slot(lobbyPlayer.slot);
        info->set_is_host(lobbyPlayer.slot == 1);
        *info->mutable_info() = lobbyPlayer.info;
    }

    pkt.set_my_lobby_id(myLobbyId);
    cout << "[Lobby] Snapshot to myLobbyId=" << myLobbyId
        << " players=" << pkt.players_size() << endl;
    session->Send(Server_PacketHandler::Make_S_LobbySnapshot(pkt));
}

void GameRoom::Broadcast_LobbySnapshot()
{
    Prune_LobbyPlayers();

    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        auto session = lobbyPlayer.session.lock();
        if (!session)
            continue;

        Send_LobbySnapshot(session);
    }
}

void GameRoom::Prune_LobbyPlayers()
{
    bool changed = false;
    for (auto iter = _lobbyPlayers.begin(); iter != _lobbyPlayers.end(); )
    {
        // 로비에 남아 있는 세션이 실제로 살아있는지 확인한다.
        auto lobbySession = iter->second.session.lock();
        if (!lobbySession || !lobbySession->IsConnected())
        {
            iter = _lobbyPlayers.erase(iter);
            changed = true;
            continue;
        }

        ++iter;
    }

    // 살아있는 플레이어만 1번부터 다시 배치해서 1번 슬롯이 항상 호스트가 되게 한다.
    uint32 nextSlot = 1;
    for (auto& [lobbyId, lobbyPlayer] : _lobbyPlayers)
    {
        if (lobbyPlayer.slot != nextSlot)
            changed = true;

        lobbyPlayer.slot = nextSlot++;
    }

    if (changed)
        cout << "[Lobby] Pruned lobby players. players=" << _lobbyPlayers.size() << endl;
}

void GameRoom::Ensure_LevelMonstersSpawned()
{
    if (_levelMonstersSpawned)
       return;

    vector<FServerMonsterSpawnDesc> spawnDescs;
    if (!Load_MonsterSpawnData_FromLevel(MAPNAME, spawnDescs))
        return;

    for (const auto& spawnDesc : spawnDescs)
    {
        auto monster = Monster::Create();
        CHECK_NULL(monster);

        monster->Initialize_FromSpawn(spawnDesc.position, spawnDesc.yaw);
        monster->info.set_objecttype(spawnDesc.objectType);
        if (!spawnDesc.prefabName.empty())
            monster->info.set_name(spawnDesc.prefabName);

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

void GameRoom::Ensure_LevelWaveTriggersLoaded()
{
    if (_levelWaveTriggersLoaded)
        return;

    _waveTriggers.clear();

    if (Load_WaveTriggerData_FromLevel(MAPNAME, _waveTriggers))
        _levelWaveTriggersLoaded = true;
}

void GameRoom::Update_WaveTriggers()
{
    for (auto& trigger : _waveTriggers)
    {
        if (trigger.hasTriggered)
        {
            if (!trigger.clearBroadcasted && Is_WaveTriggerCleared(trigger))
            {
                trigger.clearBroadcasted = true;
                Broadcast_WaveCleared(trigger.waveTag);

                // 클라이언트 WaveTrigger::Finish_WaveClear와 동일하게, 1회성이 아니면 다음 틱부터 재발동 가능하게 되돌린다.
                if (!trigger.triggerOnce)
                {
                    trigger.hasTriggered = false;
                    trigger.clearBroadcasted = false;
                    trigger.spawnedMonsterIds.clear();
                }
            }

            continue;
        }

        if (_players.empty())
        {
            trigger.playerWasInside = false;
            continue;
        }

        bool anyPlayerInside = false;

        for (const auto& [playerId, player] : _players)
        {
            const auto& pos = player->info.pos();
            const Vec3 playerPos(pos.x(), pos.y(), pos.z());

            if (!IsPointInsideWaveTrigger(playerPos, trigger))
                continue;

            anyPlayerInside = true;
            break;
        }

        // 재발동 가능한 트리거가 여전히 안쪽에 있을 때 매 틱 재발동되는 것을 막기 위해,
        // 바깥 -> 안쪽으로 새로 들어오는 순간(rising edge)에만 웨이브를 시작한다.
        if (anyPlayerInside && !trigger.playerWasInside)
            Trigger_Wave(trigger);

        trigger.playerWasInside = anyPlayerInside;
    }
}

void GameRoom::Trigger_Wave(FServerWaveTriggerDesc& trigger)
{
    if (trigger.hasTriggered)
        return;

    trigger.hasTriggered = true;
    trigger.clearBroadcasted = false;
    trigger.spawnedMonsterIds.clear();

    Broadcast_WaveStarted(trigger.waveTag);

    for (const auto& spawnEntry : trigger.spawnEntries)
    {
        auto monster = Monster::Create();
        CHECK_NULL(monster);

        const Vec3 spawnWorldPosition =
            Resolve_WaveTriggerWorldPosition(trigger.position, trigger.yaw, spawnEntry.localPosition);

        const float spawnWorldYaw =
            Resolve_WaveTriggerWorldYaw(trigger.yaw, spawnEntry.localRotation);

        monster->Initialize_FromSpawn(spawnWorldPosition, spawnWorldYaw);
        monster->info.set_objecttype(spawnEntry.objectType);
        if (!spawnEntry.prefabName.empty())
            monster->info.set_name(spawnEntry.prefabName);

        Add_Monster(monster);
        trigger.spawnedMonsterIds.push_back(monster->Get_ObjectID());
    }
}

void GameRoom::Broadcast_WaveStarted(const string& waveTag)
{
    Protocol::S_WaveStarted pkt;
    pkt.set_wave_tag(waveTag);

    Broadcast(Server_PacketHandler::Make_S_WaveStarted(pkt));
}

void GameRoom::Broadcast_WaveCleared(const string& waveTag)
{
    Protocol::S_WaveCleared pkt;
    pkt.set_wave_tag(waveTag);

    Broadcast(Server_PacketHandler::Make_S_WaveCleared(pkt));
}

bool GameRoom::Is_WaveTriggerCleared(const FServerWaveTriggerDesc& trigger) const
{
    if (trigger.spawnedMonsterIds.empty())
        return false;

    for (const uint64 monsterId : trigger.spawnedMonsterIds)
    {
        if (_monsters.find(monsterId) != _monsters.end())
            return false;
    }

    return true;
}

bool GameRoom::Load_WaveTriggerData_FromLevel(const wstring& levelName, vector<FServerWaveTriggerDesc>& outTriggers)
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

        if (objectType != Protocol::OBJECT_TYPE_WAVE_TRIGGER)
            continue;

        FServerWaveTriggerDesc trigger{};

        Try_ReadTransform(objJson, trigger.position, trigger.yaw);
        Try_ReadTriggerExtents(objJson, trigger.extents);

        if (objJson.contains("custom_properties") && objJson["custom_properties"].is_object())
        {
            const auto& custom = objJson["custom_properties"];

            if (custom.contains("wave_tag") && custom["wave_tag"].is_string())
                trigger.waveTag = custom["wave_tag"].get<string>();

            if (custom.contains("trigger_once"))
                trigger.triggerOnce = custom["trigger_once"].get<bool>();

            if (custom.contains("spawn_entries") && custom["spawn_entries"].is_array())
            {
                for (const auto& entryJson : custom["spawn_entries"])
                {
                    FServerWaveSpawnEntry entry{};

                    if (entryJson.contains("prefab_name") && entryJson["prefab_name"].is_string())
                        entry.prefabName = entryJson["prefab_name"].get<string>();

                    if (entryJson.contains("position"))
                        entry.localPosition = Json_ToVec3(entryJson["position"], Vec3(0.f, 0.f, 0.f));

                    if (entryJson.contains("rotation"))
                        entry.localRotation = Json_ToVec3(entryJson["rotation"], Vec3(0.f, 0.f, 0.f));

                    if (entryJson.contains("object_type_override"))
                    {
                        if (entryJson["object_type_override"].is_string())
                        {
                            auto enumValue = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
                                entryJson["object_type_override"].get<string>());

                            if (enumValue.has_value())
                                entry.objectType = enumValue.value();
                        }
                        else if (entryJson["object_type_override"].is_number_integer())
                        {
                            entry.objectType = static_cast<Protocol::OBJECT_TYPE>(
                                entryJson["object_type_override"].get<int32>());
                        }
                    }

                    if (!entry.prefabName.empty())
                        trigger.spawnEntries.push_back(std::move(entry));
                }
            }
        }

        if (!trigger.spawnEntries.empty())
            outTriggers.push_back(std::move(trigger));
    }

    return true;
}

