#pragma once

#include "Editor_Define.h"

NS_BEGIN(Editor)

class ImGui_Manager;
class Editor_Manager;

class EditorInstance
{
    DECLARE_SINGLETON(EditorInstance);

public:
    EditorInstance() = default;
    ~EditorInstance();

public:
    HRESULT Initialize_Editor(HWND hWnd, ComPtr<Device> device, ComPtr<DeviceContext> context);
    void    Update_Editor(float timeDelta);
    void    Render_Editor();
    void    Release();

public:
    void    Play();
    void    Pause();
    void    Stop();
    bool    IsPlaying() const { return _isPlaying && !_isPaused; }

private: /* Manager */
    unique_ptr<ImGui_Manager>   _imguiManager;
    unique_ptr<Editor_Manager>  _editorManager;

private:
    bool _isPlaying = false;
    bool _isPaused = false;

};

NS_END
