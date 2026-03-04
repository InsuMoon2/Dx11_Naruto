#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class Texture;
NS_END

NS_BEGIN(Editor)

class Content_Browser : public EditorWindow
{
private:
    struct FFileEntry
    {
        wstring filePath;
        string  guid;
    };

    struct FFolderNode
    {
        wstring name;
        wstring fullPath;

        vector<FFolderNode> subFolders;
        vector<FFileEntry>  files;
    };

public:
    explicit Content_Browser();
    virtual ~Content_Browser();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

public:
    void Load_Thumbnail(const string& guid);

private:
    void Refresh_Resources();
    void Refresh_CurrentFolder();

    void Scan_Folder(const wstring& path, FFolderNode& node);
    void Draw_FolderTree(FFolderNode& node);
    void Draw_AssetView();

    void Generate_Default_Prefabs();
    void Finish_Rename(const wstring& oldPath, const char* newName);
    void Create_NewPrefab(uint32 objectID, const wstring& typeName);

    void Enter_RenameMode(fs::path savePath);

    // 설정 저장
    void Save_Settings();
    void Load_Settings();

    FFolderNode* Find_FolderNode(FFolderNode& node, const wstring& path);
    void Expand_PathTo(const wstring& targetPath);

private:
    FFolderNode _rootFolder;
    FFolderNode* _currentFolder = { nullptr };

    // 폴더 뷰
    float _leftPanelWidth = 200.f;
    float _thumbnailSize = 64.f;
    char _searchBuffer[128] = "";

    shared_ptr<Texture> _iconFolder;
    shared_ptr<Texture> _iconFile;

private:
    bool    _isRenaming = false;
    wstring _renamingFilePath;
    char    _renameBuffer[128] = "";
    bool    _focusRenameInput = false;

    set<wstring> _expandedFolders{};

    const char* CONTENT_BROWSER_PATH =
        "../../Client/Bin/Resources/Data/json/EditorSettings/ContentBrowser.json";

    wstring _selectedFilePath;

private: /* 썸네일 */
    map<string, ComPtr<ShaderResourceView>> _thumbnailCache;
    set<string> _noThumbnailGuids;

public:
    static shared_ptr<Content_Browser> Create();
};

NS_END
