#include "pch.h"
#include "Level_Serializer.h"
#include "GameObject.h"
#include <fstream>
#include <Layer.h>

void Level_Serializer::Save_Level(const wstring& fileName, uint32 levelIndex, const wstring& levelName)
{
    wstring fullPath = Get_FullPath(fileName);

    filesystem::path path(fullPath);
    if (!filesystem::exists(path.parent_path()))
        filesystem::create_directories(path.parent_path());

    json levelJson;
    levelJson["levelName"] = Utils::ToString(levelName); // Level Name 세팅을 어떻게할지? GENERATED_LEVEL을 만들까
    levelJson["levelIndex"] = levelIndex;

    // Layer별로 순회하면서 GameObject 데이터 수집

    json objectsArray = json::array();

    const auto& layers = GAME->Get_Layers(levelIndex);

    for (auto& [layerTag, layer] : layers)
    {
        for (auto& obj : layer->Get_GameObjects())
        {
            if (obj && !obj->Is_Destroy())
            {
                objectsArray.emplace_back(GameObjectToJson(obj, layerTag));
            }
        }
    }

    levelJson["gameObjects"] = objectsArray;

    // 파일 저장
    ofstream file(fullPath);
    if (file.is_open())
    {
        file << levelJson.dump(4); // 4칸 들여쓰기
        file.close();
    }
    else
    {
        LOG_ERROR("Failed to save level: {}", Utils::ToString(fullPath));
    }
}

vector<shared_ptr<GameObject>> Level_Serializer::Load_Level(const wstring& fileName)
{
    wstring fullPath = Get_FullPath(fileName);
    vector<shared_ptr<GameObject>> objects;

    ifstream file(fullPath);
    if (!file.is_open())
    {
        LOG_ERROR("Failed to open level file: {}", Utils::ToString(fullPath));
        return objects;
    }

    json levelJson;
    file >> levelJson;
    file.close();

    if (!levelJson.contains("gameObjects"))
    {
        LOG_ERROR("Level JSON has no gameObjects array");
        return objects;
    }

    uint32 levelIndex = levelJson.value("levelIndex", GAME->Current_Level());

    string levelName = levelJson.value("levelName", "Unknown");
    LOG_INFO("Loading level: '{}' (index: {})", levelName, levelIndex);

    for (auto& objJson : levelJson["gameObjects"])
    {
        auto obj = JsonToGameObject(objJson, levelIndex);
        if (obj)
        {
            objects.emplace_back(obj);
        }
    }

    LOG_INFO("Level loaded: {} objects", objects.size());

    return objects;
}

vector<wstring> Level_Serializer::Get_SaveFiles()
{
    vector<wstring> levelFiles;

    if (!filesystem::exists(LEVEL_DIRECTORY))
    {
        filesystem::create_directories(LEVEL_DIRECTORY);
        return levelFiles;
    }

    for (const auto& entry : filesystem::directory_iterator(LEVEL_DIRECTORY))
    {
        if (entry.is_regular_file())
        {
            wstring fileName = entry.path().filename().wstring();
            if (fileName.find(L".level.json") != wstring::npos)
            {
                levelFiles.push_back(fileName);
            }
        }
    }

    return levelFiles;
}

json Level_Serializer::GameObjectToJson(shared_ptr<GameObject> obj, const wstring& layerTag)
{
    json j = obj->To_Json();

    j["layerTag"] = Utils::ToString(layerTag);

    return j;
}

shared_ptr<GameObject> Level_Serializer::JsonToGameObject(const json& j, uint32 levelIndex)
{
    // objectType
    if (!j.contains("object_type"))
    {
        LOG_WARN("Missing 'object_type' in JSON");
        return nullptr;
    }

    Protocol::OBJECT_TYPE objType = Protocol::OBJECT_TYPE_NONE;

    if (j["object_type"].is_string())
    {
        auto result = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
            j["object_type"].get<string>());
        if (!result.has_value())
        {
            LOG_WARN("Unknown object_type: '{}'. Skipping.",
                j["object_type"].get<string>());
            return nullptr;
        }

        objType = result.value();
    }

    else
    {
        objType = static_cast<Protocol::OBJECT_TYPE>(j["object_type"].get<uint32>());
    }

    // 프로토타입에서 복사
    auto gameObject = GAME->Clone_GameObject(levelIndex, objType, nullptr);
    if (!gameObject)
    {
        LOG_ERROR("Failed to clone GameObject for type: {}", magic_enum::enum_name(objType));
        return nullptr;
    }

    // 데이터 세팅
    gameObject->From_Json(j); // static_class, object_type, guid는 알아서 세팅됨

    // 그러면 컴포넌트 세팅 진행
    if (j.contains("components"))
    {
        for (const auto& compData : j["components"])
        {
            if (!compData.contains("type"))
                continue;

            uint32 typeId = 0;

            if (compData["type"].is_string())
            {
                string typeName = compData["type"].get<string>();
                auto result = magic_enum::enum_cast<Protocol::ComponentID>(typeName);

                if (!result.has_value())
                {
                    LOG_WARN("Unknown component type: '{}'. Skipping.", typeName);
                    continue;
                }
                typeId = static_cast<uint32>(result.value());
            }
            else
            {
                typeId = compData["type"].get<uint32>();
            }

            auto comp = gameObject->Find_Component_ByStaticType(typeId);

            if (comp == nullptr)
            {
                LOG_WARN("Component not found (type: {}). Skipping.",
                    compData["type"].is_string() ? compData["type"].get<string>() : to_string(typeId));

                continue;
            }

            comp->From_Json(compData);
        }
    }

    // Layer에 배치
    wstring layerTag = L"Layer_Default";

    if (j.contains("layerTag"))
    {
        layerTag = Utils::ToWString(j["layerTag"].get<string>());
    }

    HRESULT hr = GAME->Add_GameObject(levelIndex, layerTag, gameObject);
    if (FAILED(hr))
    {
        LOG_ERROR("Failed to add GameObject '{}' to layer '{}'",
            Utils::ToString(gameObject->Get_Name()),
            Utils::ToString(layerTag));
        return nullptr;
    }

    return gameObject;
}

wstring Level_Serializer::Get_FullPath(const wstring& fileName)
{
    return wstring(LEVEL_DIRECTORY) + fileName;
}
