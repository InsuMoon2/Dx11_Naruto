#pragma once

NS_BEGIN(Engine)
class RenderTarget;
class Shader;
class VIBuffer_Rect;
NS_END

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

    Shared<EditorWindow> Get_Window(const wstring& key);
    const auto& Get_Windows() const { return _windows; }

    void    Sync_RuntimeViewportForGame();

    wstring Get_LastLevelPath() const { return _lastLevelPath; }


private:
    void    Begin_DockSpace();
    void    Show_MenuBar();

    void    Show_SaveLevelDialog();
    void    Show_LoadLevelDialog();

    void    Show_DeleteConfirmModal();

    void    On_SaveLevel(const wstring& fileName);
    void    On_LoadLevel(const wstring& fileName);

    void    Resolve_RuntimeRenderTargets(Shared<RenderTarget>& sceneRT, Shared<RenderTarget>& displayRT);
    HRESULT Render_ViewportDisplayTarget(Shared<RenderTarget> sourceRT, Shared<RenderTarget> displayRT);
    HRESULT Ready_ViewportDisplayResources();

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

private:
    Shared<RenderTarget> _previewRT;
    Shared<Shader> _previewShader;
    Shared<VIBuffer_Rect> _previewRect;

    Matrix _previewWorld = Matrix::Identity;
    Matrix _previewView = Matrix::Identity;
    Matrix _previewProj = Matrix::Identity;

    Shared<Shader>          _viewportDisplayShader;
    Shared<VIBuffer_Rect>   _viewportDisplayRect;
    Matrix                  _viewportDisplayWorld = Matrix::CreateScale(2.f, 2.f, 1.f);
    Matrix                  _viewportDisplayView = Matrix::Identity;
    Matrix                  _viewportDisplayProj = Matrix::Identity;

public:
    static unique_ptr<Editor_Manager> Create();

};

NS_END
