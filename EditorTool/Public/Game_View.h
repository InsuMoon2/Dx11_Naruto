#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class RenderTarget;
NS_END

NS_BEGIN(Editor)

class Game_View final : public EditorWindow
{
public:
    explicit Game_View();
    virtual ~Game_View();

public:
    void                Initialize() override;
    void                Update(float timeDelta) override;
    void                OnGui() override;

public:
    shared_ptr<RenderTarget> Get_RenderTarget() { return _renderTarget; }

private:
    ImGuiWindowFlags    Get_WindowFlags() const;
    void                Render_Viewport();
    void                Update_WindowState();

private:
    shared_ptr<RenderTarget>    _renderTarget;

    Vec2                _viewportSize = {};
    bool                _isFocused = false;
    bool                _isHovered = false;

public:
    static shared_ptr<Game_View> Create();

};

NS_END
