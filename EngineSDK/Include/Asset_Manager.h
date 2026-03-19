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
    const FAssetMeta*   Find_ByGUID(const string& guid) const;

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

    uint32              Clear_DisallowedMeta(const wstring& directory);

private:
    bool                Load_Meta(const wstring& metaPath);
    bool                Save_Meta(const wstring& metaPath, const FAssetMeta& meta);
    string              Detect_AssetType(const wstring& filePath) const;

    void                Load_Cache();
    void                Save_Cache();

    // [추가]
    // 같은 파일을 상대/절대 경로 표기 차이로 중복 등록하지 않도록 비교용 경로를 정규화한다.
    static wstring      Normalize_PathKey(const wstring& path);

    // [추가]
    // relativePath가 있으면 그 값을 우선 기준으로, 없으면 fullPath를 기준으로 중복 여부를 판단한다.
    static wstring      Make_AssetIdentityKey(const FAssetMeta& meta);

    // [추가]
    // 현재 캐시/메타 로드 결과에서 같은 에셋 경로를 가리키는 중복 GUID를 하나로 정리한다.
    void                Deduplicate_Assets_ByPath(const string& preferGuid = "");

    // [추가]
    // _guidToMeta 상태를 기준으로 _pathToGuid 인덱스를 다시 구성한다.
    void                Rebuild_PathIndex();

    bool                Should_RegisterAsset(const fs::path& path) const;
    static bool         EndsWith(const string& value, const string& suffix);

    void                Unregister_AssetPath(const wstring& assetPath);

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
