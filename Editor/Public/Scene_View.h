#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class RenderTarget;
NS_END

NS_BEGIN(Editor)

class Scene_View : public EditorWindow
{
public:
    explicit Scene_View();
    virtual ~Scene_View();

public:
    void    Initialize() override;
    void    Update(float timeDelta) override;
    void    OnGui() override;

public:
    shared_ptr<RenderTarget> Get_RenderTarget() { return _renderTarget; }

private:
    ImGuiWindowFlags Get_WindowFlags() const;
    void             PrePare_Window();
    void             Render_Viewport();
    void             Update_WindowState();
                     
    void             ToggleFullScreen();

private:
    shared_ptr<RenderTarget>    _renderTarget;

    Vec2    _viewportSize = {};

    bool    _isFocused = false;
    bool    _isHovered = false;

    // 전체화면
    bool     _isFullScreen = false;
    RECT     _windowedRect = {};

    LONG     _savedStyle = 0;
    ImGuiID  _savedDockId = 0;       
    bool     _shouldRestoreWindow = false;
            
public:
    static shared_ptr<Scene_View> Create();


};

NS_END
