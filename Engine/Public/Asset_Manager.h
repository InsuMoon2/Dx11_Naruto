#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL Asset_Manager : public Base
{
public:
    explicit Asset_Manager();
    virtual ~Asset_Manager() = default;

public:
    HRESULT Initialize(const wstring& resourceRoot);

public: /* GUID 조회 */
    // GUID로 에셋 메타 정보 가져오기
    const FAssetMeta* Find_ByGUID(const string& guid) const;

    // 파일 경로 -> GUID
    string              Find_GUID(const wstring& filePath) const;

    // GUID -> 절대 경로
    wstring             Resolve_Path(const string& guid) const;

public: /* .meta 관리 */
    // 새 에셋에 .meta 생성
    string              Register_Asset(const wstring& filePath, const string& type = "");

    // .meta 없는 에셋 자동 감지 & 생성
    void                Scan_And_Register(const wstring& directory);

    // 에셋 타입별 목록 조회
    vector<const FAssetMeta*> Get_AssetByType(const string& type);

    void                Update_AssetPath(const string& guid, const wstring& newFilePath);
    void                Refresh_Cache();

private:
    bool                Load_Meta(const wstring& metaPath);
    bool                Save_Meta(const wstring& metaPath, const FAssetMeta& meta);
    string              Detect_AssetType(const wstring& filePath) const;

    void                Load_Cache();
    void                Save_Cache();

    

private:
    wstring                     _resourceRoot;
    wstring                     _cachePath;

    umap<string, FAssetMeta>    _guidToMeta;
    umap<wstring, string>       _pathToGuid;

    vector<string> _ignoreFolders = {"Fonts", ".git"};

public:
    static Unique<Asset_Manager> Create(const wstring& resourceRoot);
    void Free() override;

};

NS_END
