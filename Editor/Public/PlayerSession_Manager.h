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

    void    Save_SceneSnapshot();
    void Restore_SceneSnapshot();

private:
    bool    Launch_Server();
    bool    Launch_Client(int32 playerIndex, int32 totalPlayers);
    void    Calculate_WindowLayout(int32 playerIndex, int32 totalPlayers,
                                    int32& outX, int32& outY,
                                    int32& outWidth, int32& outHeight);

private:
    vector<PROCESS_INFORMATION> _activeProcesses;
    HWND _editorMainWindow = {};

private:
    json        _sceneSnapshot;
    bool        _hasSnapShot = false;

public:
    static unique_ptr<PlayerSession_Manager> Create();

};

NS_END
