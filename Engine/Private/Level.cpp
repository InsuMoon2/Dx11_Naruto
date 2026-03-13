#include "pch.h"
#include "Level.h"
#include "GameInstance.h"
#include <fstream>
#include <magic_enum/magic_enum.hpp>
#include "GameObject.h"

Level::Level(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device)
    , _context(context)
{

}

Level::~Level()
{
    
}


HRESULT Level::Initialize()
{


    return S_OK;
}

void Level::Priority_Update(float timeDelta)
{

}

void Level::Update(float timeDelta)
{

}

void Level::Late_Update(float timeDelta)
{

}

HRESULT Level::Render()
{

    return S_OK;
}

HRESULT Level::Load_LevelFromJson(const wstring& fileName)
{
    return Load_LevelChunkToLevel(
        GAME->Current_Level(),
        GAME->Current_Level(),
        fileName);
}

HRESULT Level::Load_LevelChunkToLevel(uint32 targetLevelIndex, uint32 prototypeLevelIndex, const wstring& fileName)
{
    wstring fullPath = wstring(L"../../Client/Bin/Resources/Data/json/Levels/") +
        fileName + wstring(L".level.json");

    ifstream file(fullPath);
    if (!file.is_open())
    {
        LOG_ERROR("파일 열기 실패 : {}", Utils::ToString(fileName));
        return E_FAIL;
    }

    json levelJson;
    file >> levelJson;
    file.close();

    if (!levelJson.contains("gameObjects"))
    {
        LOG_ERROR("Level JSON has no gameObjects array");
        return E_FAIL;
    }

    int loadedCount = 0;

    for (auto& objJson : levelJson["gameObjects"])
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
            objType = static_cast<Protocol::OBJECT_TYPE>(
                objJson["object_type"].get<uint32>());
        }

        if (objType == Protocol::OBJECT_TYPE_CAMERA_FREE ||
            objType == Protocol::OBJECT_TYPE_CAMERA_TARGET ||
            objType == Protocol::OBJECT_TYPE_PLAYER)
        {
            continue;
        }

        auto gameObject = GAME->Clone_GameObject(prototypeLevelIndex, objType, nullptr);

        if (!gameObject && prototypeLevelIndex != 0)
        {
            gameObject = GAME->Clone_GameObject(0, objType, nullptr);
        }

        if (!gameObject)
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
                {
                    typeId = compData["type"].get<uint32>();
                }

                auto comp = gameObject->Find_Component_ByStaticType(typeId);
                if (comp)
                    comp->From_Json(compData);
            }
        }

        wstring layerTag = L"Layer_Default";
        if (objJson.contains("layerTag"))
            layerTag = Utils::ToWString(objJson["layerTag"].get<string>());

        CHECK_FAILED(GAME->Add_GameObject(targetLevelIndex, layerTag, gameObject), E_FAIL);
        loadedCount++;
    }

    LOG_INFO("Level loaded: {} objects from '{}'", loadedCount, Utils::ToString(fileName));
    return S_OK;
}

void Level::Free()
{
    Base::Free();

}
