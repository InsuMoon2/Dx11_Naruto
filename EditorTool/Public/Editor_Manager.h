#pragma once

NS_BEGIN(Editor)

class EditorWindow;

class Editor_Manager
{
public:
    Editor_Manager() = default;
    ~Editor_Manager();

public:
    void    Initialize();
    void    Update(float timeDelta);
    void    Render();

    void    AddWindow(const wstring& key, shared_ptr<EditorWindow> window);

private:
    void    BeginDockSpace();
    void    ShowMenuBar();

private:
    umap<wstring, shared_ptr<EditorWindow>> _windows;

public:
    static unique_ptr<Editor_Manager> Create();

};

NS_END
