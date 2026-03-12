#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class Texture;
NS_END

NS_BEGIN(Editor)

enum class EIconType
{
    Folder, Csv, Json, Python, END
};

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

    void Draw_FolderTile(FFolderNode& folder);
    void Open_Folder(FFolderNode* folder);
    void Draw_FolderContextMenu(FFolderNode& folder, const string& popupId);
    void Draw_FileContextMenu(const wstring& filePath, const string& popupId);

    void Finish_Rename(const wstring& oldPath, const char* newName);
    void Create_NewPrefab(uint32 objectID, const wstring& typeName);

    void Enter_RenameMode(fs::path savePath);

    // 설정 저장
    void Save_Settings();
    void Load_Settings();

    FFolderNode* Find_FolderNode(FFolderNode& node, const wstring& path);
    void Expand_PathTo(const wstring& targetPath);

    static bool IsFolderMatchingSearch(const Content_Browser::FFolderNode& node, const string& searchStr);

    FFolderNode* Get_FirstMatchingFolder(FFolderNode& node, const string& searchStr);

    // 사이드 버튼
    void Record_FolderHistory(const wstring& path);
    bool Can_GoBack() const;
    bool Can_GoForward() const;
    void Go_BackFolder();
    void Go_ForwardFolder();

    void Handle_SideButtonEvnet();

private:
    FFolderNode _rootFolder;
    FFolderNode* _currentFolder = { nullptr };

    // 폴더 뷰
    float _leftPanelWidth = 300.f;
    float _thumbnailSize = 64.f;
    char _searchBuffer[128] = "";

    vector<Shared<Texture>> _iconFiles;

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

    /* 사이드 버튼 */
    vector<wstring> _folderHistory;
    int _folderHistoryIndex = -1;
    bool _suppressHistoryRecord = false;


public:
    static shared_ptr<Content_Browser> Create();
};

NS_END
