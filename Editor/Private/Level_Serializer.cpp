#include "pch.h"
#include "Level_Serializer.h"
#include "GameObject.h"
#include <fstream>

void Level_Serializer::Save_Level(const wstring& fileName, const vector<shared_ptr<GameObject>>& gameObjects)
{
    wstring fullPath = Get_FullPath(fileName);

    filesystem::path path(fullPath);
    if (!filesystem::exists(path.parent_path()))
    {
        filesystem::create_directories(path.parent_path());
    }

    json levelJson;
    levelJson["levelName"] = "EditorLevel";

    json objectsArray = json::array();
    for (auto& obj : gameObjects)
    {
        if (obj)
            objectsArray.emplace_back(GameObjectToJson(obj));
    }

    levelJson["gameObjects"] = objectsArray;

    // 파일 저장
    ofstream file(fullPath);
    if (file.is_open())
    {
        file << levelJson.dump(4); // 4칸 들여쓰기
        file.close();
    }
}

vector<shared_ptr<GameObject>> Level_Serializer::Load_Level(const wstring& fileName)
{
    wstring fullPath = Get_FullPath(fileName);

    vector<shared_ptr<GameObject>> objects;

    ifstream file(fullPath);
    if (!file.is_open())
    {
        return objects;
    }

    json levelJson;
    file >> levelJson;
    file.close();

    if (!levelJson.contains("gameObjects"))
        return objects;

    for (auto& objJson : levelJson["gameObjects"])
    {
        auto obj = JsonToGameObject(objJson);
        if (obj)
        {
            objects.emplace_back(obj);
        }
    }

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

json Level_Serializer::GameObjectToJson(shared_ptr<GameObject> obj)
{
    json j;
    j["name"] = Utils::ToString(obj->Get_Name());

    // TODO : Component 직렬화 추가
    //json components = json::array();

    return j;
}

shared_ptr<GameObject> Level_Serializer::JsonToGameObject(const json& j)
{
    string nameStr = j["name"];

    // TODO : GameObjectFactory로 생성
    return nullptr;
}

wstring Level_Serializer::Get_FullPath(const wstring& fileName)
{
    return wstring(LEVEL_DIRECTORY) + fileName;
}
