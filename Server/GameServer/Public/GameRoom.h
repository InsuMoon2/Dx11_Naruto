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

public: /* 게임 로직 */
    void Update(float timeDelta);

private:
    // [추가] 현재 몬스터 BT 결과를 서버로 릴레이할 권한 플레이어 id를 계산할 때 호출한다.
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
    map<uint64, Shared<Player>>     _players;
    map<uint64, Shared<Monster>>    _monsters;

    bool                            _levelMonstersSpawned = false;
};

extern Shared<GameRoom> GRoom;

NS_END
