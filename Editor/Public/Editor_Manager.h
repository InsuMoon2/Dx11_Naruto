#pragma once

NS_BEGIN(Editor)

class EditorWindow;
class Scene_View;

class Editor_Manager
{
public:
    Editor_Manager() = default;
    ~Editor_Manager();

public:
    void    Initialize();
    void    Update(float timeDelta);
    void    Render();

    void    Add_Window(const wstring& key, shared_ptr<EditorWindow> window);

public:
    // 단축키 세팅
    void    Handle_Shortcuts();

    shared_ptr<EditorWindow> Get_Window(const wstring& key);

private:
    void    Begin_DockSpace();
    void    Show_MenuBar();

    void    Show_SaveLevelDialog();
    void    Show_LoadLevelDialog();

    void    Show_DeleteConfirmModal();

    void    On_SaveLevel(const wstring& fileName);
    void    On_LoadLevel(const wstring& fileName);

private:
    umap<wstring, shared_ptr<EditorWindow>> _windows;

    bool            _showSaveLevelDialog = false;
    bool            _showLoadLevelDialog = false;

    vector<wstring> _levelFiles;
    int32           _selectedLevelIndex = -1;
    char            _levelNameBuffer[256] = "";

    wstring         _lastLevelPath = L"";
    wstring         _deleteTargetFile = L"";
    bool            _showDeleteConfirm = false;

public:
    static unique_ptr<Editor_Manager> Create();

};

NS_END
