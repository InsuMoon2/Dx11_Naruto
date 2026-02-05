#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class Texture;
NS_END

NS_BEGIN(Editor)

class Content_Browser : public EditorWindow
{
private:
    struct FFolderNode
    {
        wstring name;
        wstring fullPath;

        vector<FFolderNode> subFolders;
        vector<wstring> files;
    };

public:
    explicit Content_Browser();
    virtual ~Content_Browser();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

private:
    void    Refresh_Resources();
    void    Scan_Folder(const wstring& path, FFolderNode& node);
    void    Draw_FolderTree(FFolderNode& node);
    void    Draw_AssetView();

    void    Generate_Default_Prefabs();

private:
    FFolderNode          _rootFolder;
    FFolderNode*         _currentFolder = { nullptr };

    //
    float                _leftPanelWidth = 200.f;
    float                _thumbnailSize = 64.f;
    char                 _searchBuffer[128] = "";

    shared_ptr<Texture>  _iconFolder;
    shared_ptr<Texture>  _iconFile;

public:
    static shared_ptr<Content_Browser> Create();
};

NS_END
