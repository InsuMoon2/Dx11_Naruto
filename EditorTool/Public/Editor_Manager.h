#pragma once

NS_BEGIN(Editor)

class EditorWindow;
class SceneView;

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
    shared_ptr<EditorWindow> Get_Window(const wstring& key);

private:
    void    Begin_DockSpace();
    void    Show_MenuBar();

    void    Show_SaveLevelDialog();
    void    Show_LoadLevelDialog();
    void    On_SaveLevel(const wstring& fileName);
    void    On_LoadLevel(const wstring& fileName);

private:
    umap<wstring, shared_ptr<EditorWindow>> _windows;

    bool            _showSaveLevelDialog = false;
    bool            _showLoadLevelDialog = false;

    vector<wstring> _levelFiles;
    int32           _selectedLevelIndex = -1;
    char            _levelNameBuffer[256] = "";

public:
    static unique_ptr<Editor_Manager> Create();

};

NS_END
