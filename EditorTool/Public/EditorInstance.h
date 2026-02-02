#pragma once

#include "Editor_Define.h"
#include "Engine_Struct.h"

NS_BEGIN(Editor)
class ImGui_Manager;
class Editor_Manager;
class PlayerSession_Manager;
class EditorWindow;

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
    bool    IsPlaying() const { return _isPlaying && !_isPaused; }

    HWND    Get_WindowHandle() const { return _desc.hWnd; }

public: /* Editor Manager */
    shared_ptr<EditorWindow> Get_Window(const wstring& key);

public: /* PlayerSession Manager */
    void    Start_SinglePlayer();
    void    Start_MultiPlayer(int32 playerCount);

private: /* Manager */
    unique_ptr<ImGui_Manager>           _imguiManager;
    unique_ptr<Editor_Manager>          _editorManager;
    unique_ptr<PlayerSession_Manager>   _playerSessionManager;

private:
    bool _isPlaying = true;
    bool _isPaused = false;

    EDITOR_DESC _desc = {};

};

NS_END
