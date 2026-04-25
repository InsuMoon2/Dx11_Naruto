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
    // Editor_Manager가 Play 프레임을 그릴 때 Game View의 RT 디버그 토글 상태를 읽기 위해 호출한다.
    bool                 Should_RenderRTDebug() const { return _showRenderTargetDebug; }
    // Scene/Game View 포커스 상태에서 F2를 눌렀을 때 Game View용 RT 디버그 표시 상태를 뒤집는다.
    void                 Toggle_RenderRTDebug() { _showRenderTargetDebug = !_showRenderTargetDebug; }

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
    bool                        _showRenderTargetDebug = false; // Game View에서 MRT 디버그 오버레이를 보여줄지 결정하는 토글이다.

public:
    static Shared<Game_View> Create();

};

NS_END
