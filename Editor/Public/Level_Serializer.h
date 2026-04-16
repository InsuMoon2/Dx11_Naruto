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
    static void Save_Level(const wstring& fileName, uint32 levelIndex,
        const wstring& levelName);

    static vector<shared_ptr<GameObject>> Load_Level(const wstring& fileName);

    static vector<wstring> Get_SaveFiles();

    static wstring Get_FullPath(const wstring& fileName);

    static void Save_LevelProxy(const wstring& fileName, uint32 levelIndex, const wstring& levelName);
    static vector<shared_ptr<GameObject>> Load_LevelProxy(const wstring& fileName);
    static wstring Get_ProxyFullPath(const wstring& fileName);
    static bool Is_ProxyObject(shared_ptr<GameObject> obj);

private:
    static json GameObjectToJson(shared_ptr<GameObject> obj,
        const wstring& layerTag);
    static shared_ptr<GameObject> JsonToGameObject(const json& j,
        uint32 levelIndex);

private:
    static constexpr const wchar_t* LEVEL_DIRECTORY =
        L"../../Client/Bin/Resources/Data/json/Levels/";
};

NS_END
