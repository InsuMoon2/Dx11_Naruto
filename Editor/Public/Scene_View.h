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

    void    Pre_Render() override;


public:
    shared_ptr<RenderTarget> Get_RenderTarget() { return _renderTarget; }
    shared_ptr<RenderTarget> Get_DisplayRenderTarget() { return _displayRenderTarget; }

    void                Focus_OnPosition(const Vec3& targetPos);


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
    string              GUID_To_PrefabName(const string& guid);
    void                Spawn_Prefab(const string& guid, const Vec3& worldPos);

    void                Render_Preview();
    void                Clear_Drag();

    // Static Mesh Spawn
    Shared<GameObject>  Create_StaticMesh(const string& guid, const Vec3& position);
    void                Handle_DragDrop(Shared<GameObject> previewObj,
                                            const Vec3& worldPos,
                                            const ImGuiPayload* payload);

    // Camera
    void                Update_CameraLerp(float timeDelta);

    

private: /* Prefab Preview */
    Shared<GameObject>      _previewObject;
    bool                    _isDraggingPrefab = false;

private:
    Shared<RenderTarget>    _renderTarget;
    Shared<RenderTarget>    _displayRenderTarget;

    Vec2    _viewportSize = {};

    bool    _isHovered = false;

    // 전체화면
    bool     _isFullScreen = false;
    RECT     _windowedRect = {};

    LONG     _savedStyle = 0;
    ImGuiID  _savedDockId = 0;       
    bool     _shouldRestoreWindow = false;

private: /* Camera*/
    bool     _isCameraLerping = false;
    Vec3     _lerpTargetPos = {};
    float    _lerpDistance = 5.f; // 타겟과 카메라 간의 거리

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE      _gizmoMode = ImGuizmo::LOCAL; // 좌표계 모드

    bool                _altDragDuplicated = false;

    Vec3                _gizmoStartPos;
    Quat                _gizmoStartRot;
    Vec3                _gizmoStartScale;
    bool                _gizmoWasUsing = false;

public:
    static shared_ptr<Scene_View> Create();


};


NS_END
