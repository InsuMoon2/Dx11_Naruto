#pragma once

NS_BEGIN(Engine)
class GameObject;
NS_END

NS_BEGIN(Editor)

class Level_Serializer
{
public:
    explicit Level_Serializer() = default;
    ~Level_Serializer() = default;

public:
    static void Save_Level(const wstring& fileName, const vector<shared_ptr<GameObject>>& gameObjects);
    static vector<shared_ptr<GameObject>> Load_Level(const wstring& fileName);

    static vector<wstring> Get_SaveFiles();

    static wstring Get_FullPath(const wstring& fileName);

private:
    static json GameObjectToJson(shared_ptr<GameObject> obj);
    static shared_ptr<GameObject> JsonToGameObject(const json& j);

private:
    static constexpr const wchar_t* LEVEL_DIRECTORY = L"../../Client/Bin/Resources/Data/Level/";

};

NS_END
