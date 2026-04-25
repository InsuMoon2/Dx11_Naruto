#pragma once

#include "Server_PacketHandler.h"
#include "GameObject.h"
#include "Player.h"
#include "Monster.h"

NS_BEGIN(Server)

class GameSession;

class GameRoom : public enable_shared_from_this<GameRoom>
{
public:
    explicit GameRoom() = default;
    virtual ~GameRoom() = default;

    Shared<GameRoom> GetRoomRef() { return shared_from_this(); }

public: /* 입장, 퇴장 */
    void Enter_GameRoom(Shared<GameSession> session, const Protocol::C_EnterGame& pkt);
    void Leave_GameRoom(Shared<GameSession> session);

public: /* 오브젝트 관리 */
    void            Add_Player(Shared<Player> player)       { Add(player, _players); }
    void            Remove_Player(uint64 id)                { Remove(id, _players); }
    Shared<Player>  Find_Player(uint64 id)                  { return Find(id, _players); }

    void            Add_Monster(Shared<Monster> monster)    { Add(monster, _monsters); }
    void            Remove_Monster(uint64 id)               { Remove(id, _monsters); }
    Shared<Monster> Find_Monster(uint64 id)                 { return Find(id, _monsters); }

public: /* 패킷 핸들러 */
    void Handle_C_Move(Shared<GameSession> session, Protocol::C_Move& pkt);

public: /* 네트워크 */
    void Broadcast(SendBufferRef sendBuffer);
    void Broadcast_Lobby(SendBufferRef sendBuffer);

public: /* 게임 로직 */
    void Update(float timeDelta);

public:
    void Join_Lobby(Shared<GameSession> session, const Protocol::C_LobbyJoin& pkt);

    void Handle_LobbyChat(Shared<GameSession> session, const Protocol::C_LobbyChat& pkt);
    void Handle_LobbyStartGame(Shared<GameSession> session);

    void Send_LobbySnapshot(Shared<GameSession> session);
    void Broadcast_LobbySnapshot();

private:
    // 로비 스냅샷을 만들기 전에 끊어진 세션을 제거하고 슬롯을 1번부터 다시 맞춘다.
    // 클라이언트가 비정상 종료된 뒤 새로 접속할 때 호스트 슬롯이 밀리지 않게 호출한다.
    void Prune_LobbyPlayers();

    uint64 Get_MonsterAuthorityPlayerId() const;
    void Ensure_LevelMonstersSpawned();

    struct FServerMonsterSpawnDesc
    {
        // 레벨에 저장된 몬스터 프리팹 이름
        string prefabName;
        Protocol::OBJECT_TYPE objectType = Protocol::OBJECT_TYPE_MONSTER; // 레벨에 저장된 적 오브젝트 타입

        Vec3 position = Vec3(0.f, 0.f, 0.f);
        float yaw = 0.f;

        // 소환할 레이어
        wstring layerTag = L"Layer_GameObject";
    };

    static bool Load_MonsterSpawnData_FromLevel(const wstring& levelName, vector<FServerMonsterSpawnDesc>& outSpawns);

    struct FLobbyPlayer
    {
        // 로비에서만 사용할 임시 Id
        uint64 lobbyId = 0;

        uint32 slot = 0;

        Protocol::ObjectInfo info;
        Weak<GameSession> session;
    };

    uint64 _nextLobbyId = 1;
    map<uint64, FLobbyPlayer> _lobbyPlayers;


public:
    struct FServerWaveSpawnEntry
    {
        string prefabName;
        Protocol::OBJECT_TYPE objectType = Protocol::OBJECT_TYPE_MONSTER;
        Vec3 localPosition = Vec3(0.f, 0.f, 0.f);
        Vec3 localRotation = Vec3(0.f, 0.f, 0.f);
    };

    struct FServerWaveTriggerDesc
    {
        string waveTag;
        bool triggerOnce = false;
        bool hasTriggered = false;
        bool clearBroadcasted = false;

        Vec3 position = Vec3(0.f, 0.f, 0.f);
        float yaw = 0.f;
        Vec3 extents = Vec3(2.f, 2.f, 2.f);

        vector<FServerWaveSpawnEntry> spawnEntries;
        vector<uint64> spawnedMonsterIds;
    };

    vector<FServerWaveTriggerDesc> _waveTriggers;
    bool _levelWaveTriggersLoaded = false;

private:
    void Ensure_LevelWaveTriggersLoaded();
    void Update_WaveTriggers();
    void Trigger_Wave(FServerWaveTriggerDesc& trigger);
    void Broadcast_WaveStarted(const string& waveTag);
    void Broadcast_WaveCleared(const string& waveTag);
    bool Is_WaveTriggerCleared(const FServerWaveTriggerDesc& trigger) const;

    static bool Load_WaveTriggerData_FromLevel(
        const wstring& levelName,
        vector<FServerWaveTriggerDesc>& outTriggers);

private:
    template<typename T>
    Shared<T> Find(uint64 id, const map<uint64, shared_ptr<T>>& container)
    {
        auto iter = container.find(id);

        if (iter != container.end())
            return iter->second;

        return nullptr;
    }

    template<typename T>
    void Add(Shared<T> obj, map<uint64, Shared<T>>& container)
    {
        uint64 id = obj->Get_ObjectID();
        container[id] = obj;
        obj->room = GetRoomRef();

        Protocol::S_AddObject pkt;
        *pkt.add_objects() = obj->info;
        Broadcast(Server_PacketHandler::Make_S_AddObject(pkt));
    }

    template<typename T>
    void Remove(uint64 id, map<uint64, shared_ptr<T>>& container)
    {
        auto iter = container.find(id);
        if (iter == container.end())
            return;

        iter->second->room = nullptr;
        container.erase(iter);

        Protocol::S_RemoveObject pkt;
        pkt.add_ids(id);
        Broadcast(Server_PacketHandler::Make_S_RemoveObject(pkt));
    }

private:
    static constexpr const wchar_t* MAPNAME = L"[20260424]Tutorial";

    map<uint64, Shared<Player>>     _players;
    map<uint64, Shared<Monster>>    _monsters;

    bool                            _levelMonstersSpawned = false;
};

extern Shared<GameRoom> GRoom;

NS_END
