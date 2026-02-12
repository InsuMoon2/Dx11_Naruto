#include "pch.h"
#include "EditorInstance.h"
#include "ImGui_Manager.h"    
#include "Editor_Manager.h"
#include "Notification_Manager.h"
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
    CHECK_NULL(_imguiManager, E_FAIL);

    Inspector_Factory::GetInstance()->Initialize();

    _editorManager = Editor_Manager::Create();
    CHECK_NULL(_editorManager, E_FAIL);

    _playerSessionManager = PlayerSession_Manager::Create();
    CHECK_NULL(_playerSessionManager, E_FAIL);

    _notificationManager = Notification_Manager::Create();
    CHECK_NULL(_notificationManager, E_FAIL);

    return S_OK;
}

void EditorInstance::Update_Editor(float timeDelta)
{
    _imguiManager->Update(timeDelta);
    _editorManager->Update(timeDelta);
    //_notificationManager->Update(timeDelta);
}

void EditorInstance::Render_Editor()
{
    _editorManager->Render();
    //_notificationManager->Render();


    {
        _imguiManager->Render();
    }
}

void EditorInstance::Release()
{
    _playerSessionManager.reset();
    //_notificationManager.reset();

    // ImGui
    _editorManager.reset();
    _imguiManager.reset();
}

void EditorInstance::Play()
{
    GAME->Set_GameState(EGameState::Play);
}

void EditorInstance::Pause()
{
    GAME->Set_GameState(EGameState::Pause);
}

void EditorInstance::Stop()
{
    GAME->Set_GameState(EGameState::Edit);
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
