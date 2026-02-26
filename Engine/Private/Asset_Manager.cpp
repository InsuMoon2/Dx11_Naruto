#include "pch.h"
#include "Asset_Manager.h"
#include <fstream>

Asset_Manager::Asset_Manager()
{
}

HRESULT Asset_Manager::Initialize(const wstring& resourceRoot)
{
    _resourceRoot = resourceRoot;
    _cachePath = resourceRoot + L"/.asset_cache.json";

    if (fs::exists(_cachePath))
    {
        Load_Cache();
    }
    else
    {
        Scan_And_Register(_resourceRoot);
        Save_Cache();
    }

    return S_OK;
}

const FAssetMeta* Asset_Manager::Find_ByGUID(const string& guid) const
{
    auto iter = _guidToMeta.find(guid);
    if (iter == _guidToMeta.end())
        return nullptr;

    return &iter->second;
}

string Asset_Manager::Find_GUID(const wstring& filePath) const
{
    wstring absPath = fs::absolute(filePath).wstring();

    auto iter = _pathToGuid.find(absPath);
    if (iter == _pathToGuid.end())
        return "";

    return iter->second;
}

wstring Asset_Manager::Resolve_Path(const string& guid) const
{
    auto meta = Find_ByGUID(guid);
    if (!meta)
    {
        return L"";
    }

    return meta->fullPath;
}

string Asset_Manager::Register_Asset(const wstring& filePath, const string& type)
{
    wstring absPath = fs::absolute(filePath).wstring();

    // 이미 등록돼있으면 기존 GUID 반환
    auto existing = _pathToGuid.find(absPath);
    if (existing != _pathToGuid.end())
        return existing->second;

    // 새 GUID 생성
    FAssetMeta meta;
    meta.guid = Utils::Generate_GUID();
    meta.type = type.empty() ? Detect_AssetType(filePath) : type;
    meta.fullPath = absPath;

    // 상대 경로
    fs::path relative = fs::relative(absPath, _resourceRoot);
    meta.relativePath = relative.wstring();

    wstring metaPath = absPath + L".meta";
    Save_Meta(metaPath, meta);

    _guidToMeta[meta.guid] = meta;
    _pathToGuid[absPath] = meta.guid;

    Save_Cache();

    return meta.guid;
}

void Asset_Manager::Scan_And_Register(const wstring& directory)
{
    if (!fs::exists(directory))
        return;

    for (const auto& entry : fs::recursive_directory_iterator(directory))
    {
        if (entry.is_directory())
            continue;

        wstring filePath = entry.path().wstring();

        // .meta는 스킵
        if (entry.path().extension() == L".meta")
            continue;

        // 캐시 파일 스킵
        if (entry.path().filename() == L".asset_cache.json")
            continue;

        string pathStr = Utils::ToString(filePath);
        bool skip = false;

        for (auto& folder : _ignoreFolders)
        {
            if (pathStr.find(folder) != string::npos)
            {
                skip = true;
                break;
            }
        }

        if (skip)
            continue;

        wstring metaPath = filePath + L".meta";
        if (fs::exists(metaPath))
        {
            Load_Meta(metaPath);
        }
        else
        {
            Register_Asset(filePath);
        }

    }

}

vector<const FAssetMeta*> Asset_Manager::Get_AssetByType(const string& type)
{
    vector<const FAssetMeta*> result;

    for (const auto& [guid, meta] : _guidToMeta)
    {
        if (meta.type == type)
            result.push_back(&meta);
    }

    return result;
}

bool Asset_Manager::Load_Meta(const wstring& metaPath)
{
    ifstream file(metaPath);
    if (!file.is_open())
        return false;

    try
    {
        json root;
        file >> root;
        file.close();

        if (!root.contains("guid"))
            return false;

        wstring assetPath = metaPath.substr(0, metaPath.size() - 5); // .meta 제거
        wstring absPath = fs::absolute(assetPath).wstring();

        FAssetMeta meta;
        meta.guid = root["guid"].get<string>();
        meta.type = root.value("type", "");
        meta.fullPath = absPath;

        fs::path relative = fs::relative(absPath, _resourceRoot);
        meta.relativePath = relative.wstring();

        _guidToMeta[meta.guid] = meta;
        _pathToGuid[absPath] = meta.guid;

        return true;
    }

    catch (const exception& e)
    {
        LOG_ERROR("Failed to parse meta: {}", e.what());
        return false;
    }
}

bool Asset_Manager::Save_Meta(const wstring& metaPath, const FAssetMeta& meta)
{
    json root;
    root["guid"] = meta.guid;
    root["type"] = meta.type;

    ofstream file(metaPath);

    if (!file.is_open())
        return false;

    file << root.dump(4);
    file.close();

    return true;
}

string Asset_Manager::Detect_AssetType(const wstring& filePath) const
{
    fs::path path(filePath);
    string extension = path.extension().string();

    if (extension == ".json")
    {
        // 폴더 추가될떄마다 세팅해줘야함
        string pathStr = Utils::ToString(filePath);
        if (pathStr.find("Prefabs") != string::npos)         return "prefab";
        if (pathStr.find("BehaviorTrees") != string::npos)   return "behavior_tree";
        if (pathStr.find("Levels") != string::npos)          return "level";

        return "json";
    }

    if (extension == ".png" || extension == ".jpg" || extension == ".dds" || extension == ".tga")
        return "texture";

    if (extension == ".fbx" || extension == ".obj" || extension == ".psk")
        return "model";

    if (extension == ".hlsl" || extension == ".fx")
        return "shader";

    if (extension == ".clip" || extension == ".psa")
        return "animation";

    if (extension == ".wav" || extension == ".mp3" || extension == ".ogg")
        return "audio";

    if (extension == ".ttf" || extension == ".otf")
        return "font";

    return "unknown";
}

void Asset_Manager::Load_Cache()
{
    ifstream file(_cachePath);

    if (!file.is_open())
        return;

    try
    {
        json root;
        file >> root;
        file.close();
        for (auto& [guid, data] : root.items())
        {
            FAssetMeta meta;
            meta.guid = guid;
            meta.type = data.value("type", "");
            meta.fullPath = Utils::ToWString(data.value("fullPath", ""));
            meta.relativePath = Utils::ToWString(data.value("relativePath", ""));
            _guidToMeta[meta.guid] = meta;
            _pathToGuid[meta.fullPath] = meta.guid;
        }
    }

    catch (const exception& e)
    {
        LOG_WARN("Asset cache load failed: {}. Full rescan.", e.what());
        _guidToMeta.clear();
        _pathToGuid.clear();
    }
}

void Asset_Manager::Save_Cache()
{
    json root;

    for (const auto& [guid, meta] : _guidToMeta)
    {
        root[guid]["type"] = meta.type;
        root[guid]["fullPath"] = Utils::ToString(meta.fullPath);
        root[guid]["relativePath"] = Utils::ToString(meta.relativePath);
    }

    ofstream file(_cachePath);

    if (file.is_open())
        file << root.dump(2);
}

Unique<Asset_Manager> Asset_Manager::Create(const wstring& resourceRoot)
{
    auto instance = make_unique<Asset_Manager>();
    instance->Initialize(resourceRoot);

    return instance;
}

void Asset_Manager::Free()
{
    _guidToMeta.clear();
    _pathToGuid.clear();

    Base::Free();
}
