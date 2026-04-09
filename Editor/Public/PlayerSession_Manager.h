#pragma once

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
    void    Capture_PlaySessionObjectGuids(uint32 levelIndex);
    void    Merge_RuntimeSpawnedObjects_IntoSnapshot(uint32 levelIndex);
    bool    Can_Persist_RuntimeObject(const Shared<GameObject>& obj) const;

private:
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
    // Play 진입 시점에 이미 존재하던 오브젝트 GUID 집합이다.
    // Stop 시 이 집합에 없는 오브젝트만 런타임 생성 대상으로 본다.
    unordered_set<string>       _playSessionObjectGuids;

public:
    static Unique<PlayerSession_Manager> Create();

};

NS_END
