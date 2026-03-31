#pragma once

#include "Editor_Define.h"
#include "Engine_Struct.h"
#include "GameInstance.h"

NS_BEGIN(Engine)
class ICommand;
NS_END

NS_BEGIN(Editor)
class ImGui_Manager;
class Editor_Manager;
class PlayerSession_Manager;
class Notification_Manager;
class CommandHistory;

class AnimNotify_Inspector_Factory;

class EditorWindow;

class AnimNotify_Inspector;
class AnimNotifyState_Inspector;


class EditorInstance
{
    DECLARE_SINGLETON(EditorInstance);

public:
    EditorInstance() = default;
    ~EditorInstance();

public:
    HRESULT Initialize_Editor(const EDITOR_DESC& desc, ComPtr<Device> device, ComPtr<DeviceContext> context);
    void    Update_Editor(float timeDelta);
    void    Render_Editor();
    void    Release();

public:
    void    Play();
    void    Pause();
    void    Stop();

    bool    IsPlaying() const { return GAME->Get_GameState() == EGameState::Play; }
    bool    IsPaused()  const { return GAME->Get_GameState() == EGameState::Pause; }

    HWND    Get_WindowHandle() const { return _desc.hWnd; }

public: /* Editor Manager */
    shared_ptr<EditorWindow> Get_Window(const wstring& key);

public: /* PlayerSession Manager */
    void    Start_SinglePlayer();
    void    Start_MultiPlayer(int32 playerCount);

public: /* Notification Manager */
    Notification_Manager* Get_Notification() { return _notificationManager.get(); }

public: /* Command History */
    void    ExecuteCommand(shared_ptr<ICommand> cmd);
    void    Undo();
    void    Redo();
    void    Clear_CommandHistory();

public: /* AnimNotify_Inspector_Factory */
    Shared<AnimNotify_Inspector>        Get_NotifyInspector(const string& typeName);
    Shared<AnimNotifyState_Inspector>   Get_NotifyStateInspector(const string& typeName);

public:
    void Save_SceneSnapshot();
    void Restore_SceneSnapshot();

private: /* Manager */
    Unique<ImGui_Manager>           _imguiManager;
    Unique<Editor_Manager>          _editorManager;
    Unique<PlayerSession_Manager>   _playerSessionManager;
    Unique<Notification_Manager>    _notificationManager;

    Unique<CommandHistory>          _commandHistory;

    Unique<AnimNotify_Inspector_Factory> _animNotifyInspector_Factory;

private:
    EDITOR_DESC _desc = {};

};

NS_END
