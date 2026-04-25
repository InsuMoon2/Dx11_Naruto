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
#include "AIController.h"
#include "BehaviorTree.h"
#include "GameObject.h"
#include "WaveTrigger.h"

#include <set>

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
    Reload_CurrentLevelMonsterAssets();

    _pausedPreviousCamera.reset();
    GAME->Stop_Cinematic();
    GAME->Set_GameInputEnabled(true);
    INPUT->Reset();
    _playerSessionManager->Begin_PlaySession();

    GAME->Set_GameState(EGameState::Play);
    ImGui::SetWindowFocus("Game");

    auto targetCam = GAME->Find_Camera(Protocol::OBJECT_TYPE_CAMERA_TARGET);

    if (targetCam)
        GAME->Set_ActiveCamera(targetCam);

}

void EditorInstance::Reload_CurrentLevelMonsterAssets()
{
    const auto monsters = Collect_CurrentLevelMonsterObjects();
    if (monsters.empty())
        return;

    set<string> reloadedPrefabNames;
    set<string> failedPrefabNames;

    for (const auto& monster : monsters)
    {
        if (!monster || monster->Is_Destroy())
            continue;

        const string& prefabName = monster->Get_SourcePrefabName();
        bool allowPrefabReapply = false;

        if (!prefabName.empty() && failedPrefabNames.contains(prefabName))
        {
            allowPrefabReapply = false;
        }
        else if (!prefabName.empty() && reloadedPrefabNames.contains(prefabName))
        {
            allowPrefabReapply = true;
        }
        else if (!prefabName.empty())
        {
            const string prefabPath =
                "../../Client/Bin/Resources/Data/json/Prefabs/" + prefabName + ".prefab.json";

            if (FAILED(GAME->Load_Prefab(prefabPath)))
            {
                LOG_WARN("Editor Play hot reload skipped prefab reload: {}", prefabPath);
                failedPrefabNames.insert(prefabName);
            }
            else
            {
                reloadedPrefabNames.insert(prefabName);
                allowPrefabReapply = true;
            }
        }

        Reload_MonsterAssets(monster, allowPrefabReapply);
    }
}

vector<Shared<GameObject>> EditorInstance::Collect_CurrentLevelMonsterObjects() const
{
    vector<Shared<GameObject>> monsters;
    set<GameObject*> seenObjects;

    for (uint32 levelIndex = 0; levelIndex < ETOI(ELevelType::END); ++levelIndex)
    {
        const auto objects = GAME->Get_GameObjects(levelIndex);

        for (const auto& gameObject : objects)
        {
            if (!Is_PlayHotReloadMonster(gameObject))
                continue;

            if (!seenObjects.insert(gameObject.get()).second)
                continue;

            monsters.push_back(gameObject);
        }
    }

    return monsters;
}

bool EditorInstance::Is_PlayHotReloadMonster(const Shared<GameObject>& gameObject) const
{
    if (!gameObject || gameObject->Is_Destroy())
        return false;

    const Protocol::OBJECT_TYPE objectType = gameObject->Get_ObjectType();
    return objectType == Protocol::OBJECT_TYPE_MONSTER ||
        objectType == Protocol::OBJECT_TYPE_BOSS_PAIN ||
        objectType == Protocol::OBJECT_TYPE_WAVE_TRIGGER;
}

void EditorInstance::Reload_MonsterAssets(const Shared<GameObject>& gameObject, bool allowPrefabReapply)
{
    if (!gameObject)
        return;

    const string& prefabName = gameObject->Get_SourcePrefabName();
    if (prefabName.empty())
    {
        LOG_WARN("Editor Play hot reload skipped monster without source prefab name.");
    }
    else if (!allowPrefabReapply)
    {
        LOG_WARN("Editor Play hot reload skipped prefab reapply because the latest prefab file could not be loaded: {}",
            prefabName);
    }
    else if (FAILED(GAME->Reapply_Prefab_ToObject(gameObject)))
    {
        LOG_WARN("Editor Play hot reload failed to reapply prefab: {}", prefabName);
    }

    auto behavior = gameObject->Get_Component<BehaviorTree>();
    if (behavior && FAILED(behavior->Reload_FromBoundAsset()))
    {
        LOG_WARN("Editor Play hot reload failed to reload BehaviorTree for object type {}.",
            static_cast<uint32>(gameObject->Get_ObjectType()));
    }

    auto aiController = gameObject->Get_Component<AIController>();
    if (aiController)
        aiController->Refresh_RuntimeBindings();

    if (auto waveTrigger = dynamic_pointer_cast<Client::WaveTrigger>(gameObject))
        waveTrigger->Refresh_ForEditorPlay();
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
    GAME->Stop_Cinematic();
    GAME->Set_GameInputEnabled(true);
    INPUT->Reset();
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
    GAME->Set_GameInputEnabled(true);
    INPUT->Reset();
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

void EditorInstance::Set_RuntimeTimeScale(float timeScale)
{
    _runtimeTimeScale = Clamp_RuntimeTimeScale(timeScale);
}

void EditorInstance::Reset_RuntimeTimeScale()
{
    _runtimeTimeScale = 1.f;
}

float EditorInstance::Clamp_RuntimeTimeScale(float timeScale)
{
    if (timeScale < 0.1f)
        return 0.1f;

    if (timeScale > 3.f)
        return 3.f;

    return timeScale;
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
