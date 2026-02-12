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
    ImGuiWindowFlags    Get_WindowFlags() const;
    void                Prepare_Window();
    void                Render_Viewport();
    void                Update_WindowState();
                        
    void                ToggleFullScreen();

    void                Update_ImGuizmo();
    void                Handle_Guizmo_Shotcut();

    // Prefab Spawn
    Vec3                Screen_To_World(Vec2 screenPos);
    void                Spawn_Prefab(const wstring& prefabPath, const Vec3& worldPos);

private:
    Shared<RenderTarget>    _renderTarget;

    Vec2    _viewportSize = {};

    bool    _isFocused = false;
    bool    _isHovered = false;

    // 전체화면
    bool     _isFullScreen = false;
    RECT     _windowedRect = {};

    LONG     _savedStyle = 0;
    ImGuiID  _savedDockId = 0;       
    bool     _shouldRestoreWindow = false;

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;

    // 좌표계 모드
    // - LOCAL: 오브젝트 기준 (오브젝트가 회전하면 축도 같이 회전)
    // - WORLD: 월드 기준 (항상 XYZ 축 고정)
    ImGuizmo::MODE _gizmoMode = ImGuizmo::LOCAL;

public:
    static shared_ptr<Scene_View> Create();


};


NS_END
