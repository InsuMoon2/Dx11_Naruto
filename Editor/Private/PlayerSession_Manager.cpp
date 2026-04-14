#include "pch.h"
#include "PlayerSession_Manager.h"
#include "Layer.h"
#include "GameObject.h"
#include "Event_Manager.h"
#include "Player.h"
#include "Spawn_Helper.h"
#include "Customizer_Manager.h"
#include "EditorInstance.h"
#include "Level_Serializer.h"
#include "PartObject.h"

static bool Is_CameraObject(const Shared<GameObject>& obj)
{
    if (!obj)
        return false;

    const auto objType = obj->Get_ObjectType();

    return objType == Protocol::OBJECT_TYPE_CAMERA_FREE ||
        objType == Protocol::OBJECT_TYPE_CAMERA_TARGET;
}

static bool Is_PlayerObject(const Shared<GameObject>& obj)
{
    if (!obj)
        return false;

    return obj->Get_ObjectType() == Protocol::OBJECT_TYPE_PLAYER;
}

PlayerSession_Manager::~PlayerSession_Manager()
{
    Stop_AllSession();
}

void PlayerSession_Manager::Start_SinglePlayer()
{
    _editorMainWindow = GetActiveWindow();
    //ShowWindow(_editorMainWindow, SW_MINIMIZE);

    // 싱글 플레이어 아닐 때 예외처리
    if (!Launch_Client(0, 1))
    {
        MessageBox(nullptr, L"Failed to launch client!", L"Error", MB_OK | MB_ICONERROR);
        ShowWindow(_editorMainWindow, SW_RESTORE);
    }

}

void PlayerSession_Manager::Start_MultiPlayer(int32 playerCount)
{
    _editorMainWindow = GetActiveWindow();
    //ShowWindow(_editorMainWindow, SW_HIDE);

    // 서버 없을 때 예외처리
    if (!Launch_Server())
    {
        MessageBox(nullptr, L"Failed to launch GameServer.exe!", L"Error", MB_OK | MB_ICONERROR);
        ShowWindow(_editorMainWindow, SW_SHOW);
        return;
    }

    Sleep(2000);

    for (int32 i = 0; i < playerCount; i++)
    {
        if (!Launch_Client(i, playerCount))
        {
            MessageBox(nullptr, L"Failed to launch client!", L"Error", MB_OK | MB_ICONERROR);
            break;
        }

        Sleep(500);
    }
}

void PlayerSession_Manager::Stop_AllSession()
{
    for (auto& pi : _activeProcesses)
    {
        if (pi.hProcess)
        {
            TerminateProcess(pi.hProcess, 0);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
        }
    }

    _activeProcesses.clear();

    if (_editorMainWindow)
    {
        ShowWindow(_editorMainWindow, SW_SHOW);
        SetForegroundWindow(_editorMainWindow);
    }
}
void PlayerSession_Manager::Begin_PlaySession()
{
    const uint32 levelIndex = GAME->Current_Level();
    // 원래 엔진의 정상적인 메모리 스냅샷 백업 시스템
    Save_SceneSnapshot();
    Capture_PlaySessionObjectGuids(levelIndex);
    if (levelIndex != ETOI(ELevelType::GamePlay))
        return;
    Remove_PlaySessionPlayers(levelIndex);
    auto playerObj = Spawn_PlaySessionPlayer(levelIndex);
    if (!playerObj)
    {
        LOG_WARN("플레이어 생성 안됐음. 왜?");
    }
}



void PlayerSession_Manager::End_PlaySession()
{
    Merge_RuntimeSpawnedObjects_IntoSnapshot(GAME->Current_Level());
    Restore_SceneSnapshot();
}

bool PlayerSession_Manager::Launch_Server()
{
    fs::path currentPath = fs::current_path();
    fs::path serverPath = currentPath / L"../../Server/GameServer/Bin/GameServer.exe";

    if (!fs::exists(serverPath))
        return false;

    STARTUPINFO startInfo = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION processInfo = {};

    BOOL result = CreateProcess(
        serverPath.wstring().c_str(),
        nullptr,
        nullptr,
        nullptr,
        FALSE,
        CREATE_NEW_CONSOLE,
        nullptr,
        serverPath.parent_path().wstring().c_str(),
        &startInfo,
        &processInfo
    );

    if (!result)
        return false;

    _activeProcesses.emplace_back(processInfo);

    return true;
}

bool PlayerSession_Manager::Launch_Client(int32 playerIndex, int32 totalPlayers)
{
    fs::path currentPath = fs::current_path();
    fs::path clientPath = currentPath / L"../../Client/Bin/Client.exe";

    if (!fs::exists(clientPath))
        return false;

    int32 posX, posY, width, height;
    Calculate_WindowLayout(playerIndex, totalPlayers, posX, posY, width, height);

    wstring args = L"--player" + to_wstring(playerIndex) +
        L" --x" + to_wstring(posX) +
        L" --y" + to_wstring(posY) +
        L" --width" + to_wstring(width) +
        L" --height" + to_wstring(height) +
        L" --no-editor";

    fs::path canonicalPath = fs::canonical(clientPath);

    wstring fullCmd = L"\"" + canonicalPath.wstring() + L"\" " + args;
    vector<wchar_t> cmdBuf(fullCmd.begin(), fullCmd.end());
    cmdBuf.push_back(L'\0');

    STARTUPINFO startInfo = { sizeof(STARTUPINFO) };
    PROCESS_INFORMATION processInfo = {};

    BOOL result = CreateProcess(
        nullptr,
        cmdBuf.data(),
        nullptr,
        nullptr,
        FALSE,
        0,
        nullptr,
        canonicalPath.parent_path().wstring().c_str(),
        &startInfo,
        &processInfo
    );

    if (!result)
        return false;
    _activeProcesses.emplace_back(processInfo);

    return true;
}

void PlayerSession_Manager::Calculate_WindowLayout(int32 playerIndex, int32 totalPlayers, int32& outX, int32& outY,
    int32& outWidth, int32& outHeight)
{
    int32 screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int32 screenHeight = GetSystemMetrics(SM_CYSCREEN);

    if (totalPlayers == 1)
    {
        outWidth = 1280;
        outHeight = 720;
        outX = (screenWidth - outWidth) / 2;
        outY = (screenHeight - outHeight) / 2;
    }
    else if (totalPlayers == 2)
    {
        outWidth = screenWidth / 2 - 40;
        outHeight = screenHeight - 150;
        outX = playerIndex * (screenWidth / 2) + 10;
        outY = 50;
    }
    else if (totalPlayers <= 4)
    {
        outWidth = screenWidth / 2 - 20;
        outHeight = screenHeight / 2 - 50;
        int32 col = playerIndex % 2;
        int32 row = playerIndex / 2;
        outX = col * (screenWidth / 2) + 10;
        outY = row * (screenHeight / 2) + 30;
    }
    else
    {
        int32 cols = static_cast<int32>(ceil(sqrt(static_cast<float>(totalPlayers))));
        int32 rows = static_cast<int32>(ceil(static_cast<float>(totalPlayers) / cols));
        outWidth = screenWidth / cols - 20;
        outHeight = screenHeight / rows - 50;
        int32 col = playerIndex % cols;
        int32 row = playerIndex / cols;
        outX = col * (screenWidth / cols) + 10;
        outY = row * (screenHeight / rows) + 30;
    }
}

void PlayerSession_Manager::Save_SceneSnapshot()
{
    uint32 levelIndex = GAME->Current_Level();
    const auto& layers = GAME->Get_Layers(levelIndex);

    _sceneSnapshot = json::object();
    _sceneSnapshot["levelIndex"] = levelIndex;

    json objectsArray = json::array();

    for (auto& [layerTag, layer] : layers)
    {
        for (auto& obj : layer->Get_GameObjects())
        {
            if (!obj || obj->Is_Destroy()) continue;

            auto objType = obj->Get_ObjectType();

            if (objType == Protocol::OBJECT_TYPE_CAMERA_FREE ||
                objType == Protocol::OBJECT_TYPE_CAMERA_TARGET ||
                objType == Protocol::OBJECT_TYPE_PLAYER)
                continue;

            if (obj->Should_ExcludeFromEditorSnapshot())
                continue;

            json j = obj->To_Json();
            j["layerTag"] = Utils::ToString(layerTag);

            objectsArray.push_back(j);
        }
    }

    _sceneSnapshot["gameObjects"] = objectsArray;
    _hasSnapShot = true;
}

void PlayerSession_Manager::Capture_PlaySessionObjectGuids(uint32 levelIndex)
{
    _playSessionObjectGuids.clear();

    const auto& layers = GAME->Get_Layers(levelIndex);

    for (const auto& [layerTag, layer] : layers)
    {
        if (!layer)
            continue;

        for (const auto& obj : layer->Get_GameObjects())
        {
            if (!obj || obj->Is_Destroy())
                continue;

            _playSessionObjectGuids.insert(obj->Get_GUID());
        }
    }
}

bool PlayerSession_Manager::Can_Persist_RuntimeObject(const Shared<GameObject>& obj) const
{
    if (!obj || obj->Is_Destroy())
        return false;

    return obj->Get_ObjectType() == Protocol::OBJECT_TYPE_MONSTER;
}

void PlayerSession_Manager::Merge_RuntimeSpawnedObjects_IntoSnapshot(uint32 levelIndex)
{
    if (!_hasSnapShot)
        return;

    if (!_sceneSnapshot.contains("gameObjects") || !_sceneSnapshot["gameObjects"].is_array())
        _sceneSnapshot["gameObjects"] = json::array();

    const auto& layers = GAME->Get_Layers(levelIndex);

    for (const auto& [layerTag, layer] : layers)
    {
        if (!layer)
            continue;

        for (const auto& obj : layer->Get_GameObjects())
        {
            if (!Can_Persist_RuntimeObject(obj))
                continue;

            if (_playSessionObjectGuids.contains(obj->Get_GUID()))
                continue;

            json objJson = obj->To_Json();
            objJson["layerTag"] = Utils::ToString(layerTag);

            _sceneSnapshot["gameObjects"].push_back(objJson);
        }
    }
}

void PlayerSession_Manager::Remove_PlaySessionPlayers(uint32 levelIndex)
{
    const auto& layers = GAME->Get_Layers(levelIndex);
    vector<shared_ptr<GameObject>> removeTargets;

    for (const auto& [layerTag, layer] : layers)
    {
        if (!layer)
            continue;

        for (const auto& obj : layer->Get_GameObjects())
        {
            if (Is_PlayerObject(obj))
                removeTargets.push_back(obj);
        }
    }

    for (const auto& obj : removeTargets)
    {
        GAME->Delete_GameObject(levelIndex, obj);
    }

}

bool PlayerSession_Manager::Find_GetPlayerSpawnTransform(uint32 levelIndex, Vec3& outSpawnPos, Quat& outSpawnRot) const
{
    outSpawnPos = Vec3::Zero;
    outSpawnRot = Quat::Identity;

    const auto allObjects = GAME->Get_GameObjects(levelIndex);

    for (const auto& obj : allObjects)
    {
        if (!obj)
            continue;

        if (obj->Get_ObjectType() != Protocol::OBJECT_TYPE_PLAYER_START)
            continue;

        auto transform = obj->Get_Transform();
        CHECK_NULL(transform, false);

        outSpawnPos = transform->Get_WorldPosition();
        outSpawnRot = transform->Get_WorldRotation();

        return true;
    }

    return false;
}

Shared<GameObject> PlayerSession_Manager::Spawn_PlaySessionPlayer(uint32 levelIndex)
{
    Vec3 spawnPos = Vec3::Zero;
    Quat spawnRot = Quat::Identity;

    // 플레이어 스타트가 있으면 그 위치 사용
    Find_GetPlayerSpawnTransform(levelIndex, spawnPos, spawnRot);

    auto playerObj = Spawn_Helper::Prefab("TestPlayer2")
        .AtLevel(levelIndex)
        .InLayer(TEXT("Layer_Player"))
        .Position(spawnPos)
        .Spawn();

    CHECK_NULL(playerObj, nullptr);

    auto transform = playerObj->Get_Transform();
    if (transform)
    {
        transform->Set_LocalPosition(spawnPos);
        transform->Set_LocalRotation(spawnRot);
    }

    Apply_PlayerCustomizing(playerObj);

    auto player = dynamic_pointer_cast<Player>(playerObj);
    if (player)
    {
        GAME->Get_DelegateHub().OnPlayerSpawned.Broadcast(player->Get_Transform());
        GAME->Get_DelegateHub().OnPlayerObjectSpawned.Broadcast(player);
    }

    return playerObj;
}

void PlayerSession_Manager::Apply_PlayerCustomizing(const Shared<GameObject>& playerObj) const
{
    auto player = dynamic_pointer_cast<Player>(playerObj);
    CHECK_NULL(player);

    auto customizer = GET_SINGLE(Customizer_Manager);
    const auto& customDesc = customizer->Get_CustomizerDesc();

    for (int32 i = 0; i < ETOI(ContainerObject::EPartSlot::END); ++i)
    {
        const wstring& partTag = customDesc.partTags[i];
        if (partTag.empty())
            continue;

        const auto slot = static_cast<ContainerObject::EPartSlot>(i);

        player->Apply_CustomizingPart(slot, partTag);

        const json& transformData = customDesc.partTransforms[i];
        if (!transformData.is_object())
            continue;

        auto partObject = player->Get_PartObject(slot);
        if (!partObject)
            continue;

        auto transform = partObject->Get_Component<Transform>();
        if (!transform)
            continue;

        transform->From_Json(transformData);
        player->Set_PartTransformOverride(slot, transformData);
    }
}

void PlayerSession_Manager::Restore_SceneSnapshot()
{
    if (!_hasSnapShot)
        return;

    uint32 levelIndex = _sceneSnapshot.value("levelIndex", GAME->Current_Level());
    const auto& layers = GAME->Get_Layers(levelIndex);

    vector<pair<Shared<GameObject>, wstring>> preserved;  // 보존할 오브젝트 + 레이어태그

    for (auto& [layerTag, layer] : layers)
    {
        for (auto& obj : layer->Get_GameObjects())
        {
            if (!obj) continue;

            auto objType = obj->Get_ObjectType();

            if (objType == Protocol::OBJECT_TYPE_CAMERA_FREE ||
                objType == Protocol::OBJECT_TYPE_CAMERA_TARGET)
            {
                preserved.push_back({ obj, layerTag });
            }
        }
    }

    // 현재 월드 다 비우고
    GAME->Clear_Layers(levelIndex);

    // 카메라만 다시 세팅
    for (auto& [obj, layerTag] : preserved)
    {
        GAME->Add_GameObject(levelIndex, layerTag, obj);
    }

    // Play 직전에 저장해 둔 GameObject들 복원
    for (auto& objJson : _sceneSnapshot["gameObjects"])
    {
        if (!objJson.contains("object_type"))
            continue;

        Protocol::OBJECT_TYPE objType = Protocol::OBJECT_TYPE_NONE;

        if (objJson["object_type"].is_string())
        {
            auto result = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
                objJson["object_type"].get<string>());

            if (!result.has_value())
                continue;

            objType = result.value();
        }
        else
        {
            objType = static_cast<Protocol::OBJECT_TYPE>(objJson["object_type"].get<uint32>());
        }

        if (objType == Protocol::OBJECT_TYPE_CAMERA_FREE ||
            objType == Protocol::OBJECT_TYPE_CAMERA_TARGET ||
            objType == Protocol::OBJECT_TYPE_PLAYER)
        {
            continue;
        }

        auto gameObject = GAME->Clone_GameObject(levelIndex, objType, nullptr);

        if (!gameObject)
            gameObject = GAME->Clone_GameObject(0, objType, nullptr);

        if (!gameObject)
            continue;

        if (gameObject->Should_ExcludeFromEditorSnapshot())
            continue;

        gameObject->From_Json(objJson);

        if (objJson.contains("components"))
        {
            for (const auto& compData : objJson["components"])
            {
                if (compData.is_null()) continue;
                if (!compData.contains("type")) continue;

                uint32 typeId = 0;

                if (compData["type"].is_string())
                {
                    auto result = magic_enum::enum_cast<Protocol::ComponentID>(
                        compData["type"].get<string>());

                    if (!result.has_value()) continue;
                    typeId = static_cast<uint32>(result.value());
                }
                else
                    typeId = compData["type"].get<uint32>();

                auto comp = gameObject->Find_Component_ByStaticType(typeId);
                if (comp) comp->From_Json(compData);
            }
        }
        wstring layerTag = L"Layer_Default";

        if (objJson.contains("layerTag"))
            layerTag = Utils::ToWString(objJson["layerTag"].get<string>());

        GAME->Add_GameObject(levelIndex, layerTag, gameObject);
    }

    _hasSnapShot = false;
    _playSessionObjectGuids.clear();
}

unique_ptr<PlayerSession_Manager> PlayerSession_Manager::Create()
{
    return make_unique<PlayerSession_Manager>();
}
