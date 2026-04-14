#include "pch.h"
#include "EditorInstance.h"
#include "Camera.h"
#include "AnimNotify_Inspector_Factory.h"
#include "BehaviorTree_View.h"
#include "ImGui_Manager.h"    
#include "Editor_Manager.h"
#include "Notification_Manager.h"
#include "PlayerSession_Manager.h"
#include "Camera_Target.h"
#include "Camera_Free.h"
#include "CommandHistory.h"

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

    GAME->Set_ImGuiContext(ImGui::GetCurrentContext());

    Inspector_Factory::GetInstance()->Initialize();

    _editorManager = Editor_Manager::Create();
    CHECK_NULL(_editorManager, E_FAIL);

    _playerSessionManager = PlayerSession_Manager::Create();
    CHECK_NULL(_playerSessionManager, E_FAIL);

    _notificationManager = Notification_Manager::Create();
    CHECK_NULL(_notificationManager, E_FAIL);

    _commandHistory = CommandHistory::Create();
    CHECK_NULL(_commandHistory, E_FAIL);

    _animNotifyInspector_Factory = AnimNotify_Inspector_Factory::Create();
    CHECK_NULL(_animNotifyInspector_Factory, E_FAIL);

    ImGui::SetWindowFocus("Game");

    return S_OK;
}

void EditorInstance::Update_Editor(float timeDelta)
{
    _imguiManager->Update(timeDelta);
    _editorManager->Update(timeDelta);
    _notificationManager->Update(timeDelta);
}

void EditorInstance::Render_Editor()
{
    _editorManager->Render();
    _notificationManager->Render();

    {
        _imguiManager->Render();
    }
}

void EditorInstance::Release()
{
    _playerSessionManager.reset();
    _notificationManager.reset();

    // ImGui
    _editorManager.reset();
    _imguiManager.reset();
}

void EditorInstance::Play()
{
    _pausedPreviousCamera.reset();
    _playerSessionManager->Begin_PlaySession();

    GAME->Set_GameState(EGameState::Play);
    ImGui::SetWindowFocus("Game");

    auto targetCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_TARGET);

    if (targetCam)
        GAME->Set_ActiveCamera(targetCam);

}

void EditorInstance::Pause()
{
    if (GAME->Get_GameState() != EGameState::Play)
        return;

    _pausedPreviousCamera = GAME->Get_ActiveCamera();

    auto freeCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_FREE);
    auto previousCam = _pausedPreviousCamera.lock();
    if (freeCam)
    {
        if (previousCam && previousCam != freeCam)
        {
            auto srcTransform = previousCam->Get_Component<Transform>();
            auto destTransform = freeCam->Get_Component<Transform>();
            if (srcTransform && destTransform)
            {
                destTransform->Set_LocalPosition(srcTransform->Get_WorldPosition());
                destTransform->Set_LocalRotation(srcTransform->Get_WorldRotation());
            }
        }

        GAME->Set_ActiveCamera(freeCam);
    }

    GAME->Set_GameState(EGameState::Pause);
    ImGui::SetWindowFocus("Scene");
}

void EditorInstance::Stop()
{
    _pausedPreviousCamera.reset();
    GAME->Set_GameState(EGameState::Edit);

    _playerSessionManager->End_PlaySession();

    Clear_CommandHistory();  // 커맨드도 초기화


    ImGui::SetWindowFocus("Scene");

    auto freeCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_FREE);
    if (freeCam)
    {
        auto targetCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_TARGET);
        if (targetCam)
        {
            auto srcTransform = targetCam->Get_Component<Transform>();
            auto destTransform = freeCam->Get_Component<Transform>();
            if (srcTransform && destTransform)
                destTransform->Set_LocalPosition(srcTransform->Get_WorldPosition());
        }

        GAME->Set_ActiveCamera(freeCam);
        INPUT->UnlockMouse();
    }

    auto btView = dynamic_pointer_cast<BehaviorTree_View>(Get_Window(TEXT("BehaviorTree")));
    if (btView && btView->Is_DebugMode())
    {
        btView->Clear_DebugMode();
    }
}

void EditorInstance::Resume()
{
    GAME->Set_GameState(EGameState::Play);
    ImGui::SetWindowFocus("Game");

    auto previousCam = _pausedPreviousCamera.lock();
    if (previousCam)
    {
        GAME->Set_ActiveCamera(previousCam);
        _pausedPreviousCamera.reset();
        return;
    }

    auto targetCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_TARGET);
    if (targetCam)
        GAME->Set_ActiveCamera(targetCam);
}

shared_ptr<EditorWindow> EditorInstance::Get_Window(const wstring& key)
{
    return _editorManager->Get_Window(key);
}

wstring EditorInstance::Get_LastLevelPath()
{
    return _editorManager->Get_LastLevelPath();
}

void EditorInstance::Start_SinglePlayer()
{
    return _playerSessionManager->Start_SinglePlayer();
}

void EditorInstance::Start_MultiPlayer(int32 playerCount)
{
    return _playerSessionManager->Start_MultiPlayer(playerCount);
}

void EditorInstance::ExecuteCommand(shared_ptr<ICommand> cmd)
{
    _commandHistory->Execute(cmd);

    for (auto& [key, window] : _editorManager->Get_Windows())
    {
        if (window && window->IsFocused())
        {
            window->MarkDirty();
            break;
        }
    }
}

void EditorInstance::Undo()
{
    return _commandHistory->Undo();
}

void EditorInstance::Redo()
{
    return _commandHistory->Redo();
}

void EditorInstance::Clear_CommandHistory()
{
    return _commandHistory->Clear();
}

Shared<AnimNotify_Inspector> EditorInstance::Get_NotifyInspector(const string& typeName)
{
    return _animNotifyInspector_Factory->Get_NotifyInspector(typeName);
}

Shared<AnimNotifyState_Inspector> EditorInstance::Get_NotifyStateInspector(const string& typeName)
{
    return _animNotifyInspector_Factory->Get_NotifyStateInspector(typeName);
}
