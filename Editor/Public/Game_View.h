#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class RenderTarget;
NS_END

NS_BEGIN(Editor)

class Game_View final : public EditorWindow
{
public:
    explicit             Game_View();
    virtual             ~Game_View();

public:
    void                Initialize() override;
    void                Update(float timeDelta) override;
    void                OnGui() override;

public:
    Shared<RenderTarget> Get_RenderTarget() { return _renderTarget; }

    bool                 Is_Focused() const { return _isFocused; }

    Shared<RenderTarget> Get_DisplayRenderTarget() { return _displayRenderTarget; }

private:
    ImGuiWindowFlags    Get_WindowFlags() const;
    void                Render_Viewport();
    void                Update_WindowState();

private:
    Shared<RenderTarget>        _renderTarget;
    Shared<RenderTarget>        _displayRenderTarget;

    Vec2                        _viewportSize = {};
    bool                        _isFocused = false;
    bool                        _isHovered = false;

public:
    static Shared<Game_View> Create();

};

NS_END
