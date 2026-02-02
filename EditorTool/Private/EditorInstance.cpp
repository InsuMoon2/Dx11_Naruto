#include "pch.h"
#include "EditorInstance.h"
#include "ImGui_Manager.h"    
#include "Editor_Manager.h"
#include "PlayerSession_Manager.h"

IMPLEMENT_SINGLETON(EditorInstance)

EditorInstance::~EditorInstance()
{
    Release();
}

HRESULT EditorInstance::Initialize_Editor(const EDITOR_DESC& desc, ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    _desc = desc;

    _imguiManager = ImGui_Manager::Create(desc.hWnd, device, context);
    CHECK_NULL_RETURN(_imguiManager, E_FAIL);

    _editorManager = Editor_Manager::Create();
    CHECK_NULL_RETURN(_editorManager, E_FAIL);

    _playerSessionManager = PlayerSession_Manager::Create();
    CHECK_NULL_RETURN(_playerSessionManager, E_FAIL);


    return S_OK;
}

void EditorInstance::Update_Editor(float timeDelta)
{
    _imguiManager->Update(timeDelta);
    _editorManager->Update(timeDelta);
}

void EditorInstance::Render_Editor()
{
    _editorManager->Render();
    _imguiManager->Render();
}

void EditorInstance::Release()
{
    
}

void EditorInstance::Play()
{
    _isPlaying = true;
    _isPaused = false;
}

void EditorInstance::Pause()
{
    _isPaused = !_isPaused;
}

void EditorInstance::Stop()
{
    _isPlaying = false;
    _isPaused = false;
}

shared_ptr<EditorWindow> EditorInstance::Get_Window(const wstring& key)
{
    return _editorManager->Get_Window(key);
}

void EditorInstance::Start_SinglePlayer()
{
    return _playerSessionManager->Start_SinglePlayer();
}

void EditorInstance::Start_MultiPlayer(int32 playerCount)
{
    return _playerSessionManager->Start_MultiPlayer(playerCount);
}
