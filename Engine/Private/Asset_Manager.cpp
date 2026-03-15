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
        Refresh_Cache();
        Scan_And_Register(_resourceRoot);
        Save_Cache();
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
    if (!Should_RegisterAsset(fs::path(filePath)))
        return "";

    wstring absPath = fs::absolute(filePath).wstring();

    // 이미 등록돼있으면 기존 GUID 반환
    auto existing = _pathToGuid.find(absPath);
    if (existing != _pathToGuid.end())
        return existing->second;

    // 새 GUID 생성
    FAssetMeta meta;
    meta.guid = Utils::Generate_GUID();
    meta.type = type.empty() ? Detect_AssetType(filePath) : type;

    if (meta.type == "model" && meta.modelType.empty())
    {
        meta.modelType = "StaticMesh";
    }

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

        const fs::path filePath = entry.path();

        string pathStr = Utils::ToString(filePath.wstring());
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

        if (!Should_RegisterAsset(filePath))
            continue;

        const wstring assetPath = filePath.wstring();
        const wstring metaPath = assetPath + L".meta";

        if (fs::exists(metaPath))
            Load_Meta(metaPath);
        else
            Register_Asset(assetPath);
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

void Asset_Manager::Update_AssetPath(const string& guid, const wstring& newFilePath)
{
    auto iter = _guidToMeta.find(guid);
    if (iter == _guidToMeta.end())
        return;

    // 기존 경로 찾아서 갱신
    wstring oldPath = iter->second.fullPath;
    _pathToGuid.erase(oldPath);

    wstring absNewPath = fs::absolute(newFilePath).wstring();
    iter->second.fullPath = absNewPath;
    iter->second.relativePath = fs::relative(absNewPath, _resourceRoot).wstring();

    _pathToGuid[absNewPath] = guid;

    Save_Cache();
}

void Asset_Manager::Refresh_Cache()
{
    vector<string> invalidGuids;

    // 존재하지 않는 파일 검사
    for (const auto& [guid, meta] : _guidToMeta)
    {
        if (!fs::exists(meta.fullPath))
        {
            invalidGuids.push_back(guid);
        }
    }

    // 무효화된 에셋 삭제
    for (const string& guid : invalidGuids)
    {
        wstring path = _guidToMeta[guid].fullPath;
        _pathToGuid.erase(path);
        _guidToMeta.erase(guid);
    }

    if (!invalidGuids.empty())
    {
        Save_Cache(); // 캐시 파일 다시 쓰기
        LOG_INFO("{} 개의 유실된 에셋을 레지스트리에서 정리", invalidGuids.size());
    }

}

uint32 Asset_Manager::Clear_DisallowedMeta(const wstring& directory)
{
    if (!fs::exists(directory))
        return 0;

    uint32 removedCount = 0;

    for (const auto& entry : fs::recursive_directory_iterator(directory))
    {
        if (entry.is_directory())
            continue;

        const fs::path path = entry.path();
        if (Utils::ToLowerCopy(path.extension().string()) != ".meta")
            continue;

        const fs::path assetPath = path.parent_path() / path.stem();
        const bool assetExists = fs::exists(assetPath);
        const bool shouldKeepMeta = assetExists && Should_RegisterAsset(assetPath);

        if (shouldKeepMeta)
            continue;

        Unregister_AssetPath(assetPath.wstring());
        fs::remove(path);
        ++removedCount;
    }

    Save_Cache();
    return removedCount;
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
        meta.modelType = root.value("modelType", "");
        meta.fullPath = absPath;

        fs::path relative = fs::relative(absPath, _resourceRoot);
        meta.relativePath = relative.wstring();

        _guidToMeta[meta.guid] = meta;
        _pathToGuid[absPath] = meta.guid;

        return true;
    }

    catch (const exception& e)
    {
        //LOG_ERROR("Failed to parse meta: {}", e.what());
        return false;
    }
}

bool Asset_Manager::Save_Meta(const wstring& metaPath, const FAssetMeta& meta)
{
    json root;
    root["guid"] = meta.guid;
    root["type"] = meta.type;

    if (!meta.modelType.empty())
        root["modelType"] = meta.modelType;

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

    string filename = Utils::ToLowerCopy(path.filename().string());

    if (EndsWith(filename, ".prefab.json"))     return "prefab";
    if (EndsWith(filename, ".bt.json"))         return "behavior_tree";
    if (EndsWith(filename, ".uianim.json"))     return "ui_animation";
    if (EndsWith(filename, ".level.json"))      return "level";
    if (EndsWith(filename, ".matinst.json"))    return "material_instance";

    string extension = Utils::ToLowerCopy(path.extension().string());

    if (extension == ".png" || extension == ".jpg" || extension == ".dds" || extension == ".tga")
        return "texture";

    if (extension == ".meshbin")
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
            meta.modelType = data.value("modelType", "");
            meta.relativePath = Utils::ToWString(data.value("relativePath", ""));
            meta.fullPath = (fs::path(_resourceRoot) / meta.relativePath).wstring();

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

        if (!meta.modelType.empty())
            root[guid]["modelType"] = meta.modelType;

        root[guid]["fullPath"] = Utils::ToString(meta.fullPath);
        root[guid]["relativePath"] = Utils::ToString(meta.relativePath);
    }

    ofstream file(_cachePath);

    if (file.is_open())
        file << root.dump(2);
}

bool Asset_Manager::Should_RegisterAsset(const fs::path& path) const
{
    if (path.empty())
        return false;

    const string filename = Utils::ToLowerCopy(path.filename().string());
    const string ext = Utils::ToLowerCopy(path.extension().string());

    if (ext == ".meta")
        return false;

    if (filename == ".asset_cache.json")
        return false;

    if (ext == ".py")
        return false;

    if (ext == ".bat")
        return false;

    // 에디터/런타임이 직접 참조하는 JSON만 등록
    if (EndsWith(filename, ".prefab.json"))
        return true;

    if (EndsWith(filename, ".bt.json"))
        return true;

    if (EndsWith(filename, ".level.json"))
        return true;

    if (EndsWith(filename, ".uianim.json"))
        return true;

    if (EndsWith(filename, ".matinst.json"))
        return true;

    // 중간 산출물/소스 파일 제외
    if (EndsWith(filename, ".material.json"))
        return false;

    if (ext == ".bin")
        return false;
    if (ext == ".gltf")
        return false;
    if (ext == ".fbx")
        return false;
    if (ext == ".obj")
        return false;
    if (ext == ".psk")
        return false;

    // 런타임에서 직접 쓰는 모델만 등록
    if (ext == ".meshbin")
        return true;

    // 텍스처
    if (ext == ".png" || ext == ".jpg" || ext == ".dds" || ext == ".tga")
        return true;

    // 셰이더
    if (ext == ".hlsl" || ext == ".fx")
        return true;

    // 애니메이션
    if (ext == ".clip" || ext == ".psa")
        return true;

    // 오디오
    if (ext == ".wav" || ext == ".mp3" || ext == ".ogg")
        return true;

    // 폰트
    if (ext == ".ttf" || ext == ".otf")
        return true;

    return false;
}

bool Asset_Manager::EndsWith(const string& value, const string& suffix)
{
    if (value.size() < suffix.size())
        return false;

    return value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void Asset_Manager::Unregister_AssetPath(const wstring& assetPath)
{
    const wstring absPath = fs::absolute(assetPath).wstring();

    auto pathIter = _pathToGuid.find(absPath);
    if (pathIter == _pathToGuid.end())
        return;

    const string guid = pathIter->second;
    _pathToGuid.erase(pathIter);
    _guidToMeta.erase(guid);
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
