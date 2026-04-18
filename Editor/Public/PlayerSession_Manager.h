#pragma once

NS_BEGIN(Engine)
class Level;
NS_END

NS_BEGIN(Editor)

class PlayerSession_Manager
{
public:
    explicit PlayerSession_Manager() = default;
    ~PlayerSession_Manager();

public:
    void    Start_SinglePlayer();
    void    Start_MultiPlayer(int32 playerCount);
    void    Stop_AllSession();

public:
    void    Begin_PlaySession();
    void    End_PlaySession();

    void    Restore_SceneSnapshot();
    void    Save_SceneSnapshot();

private:

private:
    // 현재 편집 중인 레벨이 플레이 세션용 로컬 플레이어 스폰을 허용하는지 판별한다.
    // Play 진입 시 GamePlay/Konoha 같은 실제 플레이 레벨인지 확인할 때 호출된다.
    bool    Should_SpawnPlaySessionPlayer(uint32 levelIndex) const;

    // 레벨에 있는 플레이어 제거
    void    Remove_PlaySessionPlayers(uint32 levelIndex);
    // Player Start 기준으로 스폰 위치, 회전값 세팅
    bool    Find_GetPlayerSpawnTransform(uint32 levelIndex , Vec3& outSpawnPos, Quat& outSpawnRot) const;

    Shared<GameObject> Spawn_PlaySessionPlayer(uint32 levelIndex);

    void    Apply_PlayerCustomizing(const Shared<GameObject>& playerObj) const;

private:
    bool    Launch_Server();
    bool    Launch_Client(int32 playerIndex, int32 totalPlayers);
    void    Calculate_WindowLayout(int32 playerIndex, int32 totalPlayers,
                                    int32& outX, int32& outY,
                                    int32& outWidth, int32& outHeight);

private:
    vector<PROCESS_INFORMATION> _activeProcesses;
    HWND                        _editorMainWindow = {};

private:
    json                        _sceneSnapshot;
    bool                        _hasSnapShot = false;

    // Play 진입 직전의 레벨 인덱스다.
    // Stop 시 플레이 중 레벨이 바뀌었더라도 원래 편집 레벨을 되돌릴 때 사용한다.
    uint32                      _snapshotLevelIndex = ETOI(ELevelType::Static);

    // Play 진입 직전의 레벨 객체다.
    // Stop 시 Change_Level로 원래 편집 레벨을 다시 활성화할 때 사용한다.
    Shared<Engine::Level>       _snapshotLevel;

    // Play 진입 시점에 이미 존재하던 오브젝트 GUID 집합이다.
    // Stop 시 이 집합에 없는 오브젝트만 런타임 생성 대상으로 본다.
public:
    static Unique<PlayerSession_Manager> Create();

};

NS_END
