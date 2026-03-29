#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
class RenderTarget;
class Model;
class ContainerObject;
class Transform;
NS_END

NS_BEGIN(Editor)
class Prefab_PreviewCameraSettings;
class Editor_Camera_Free;

class Prefab_View : public EditorWindow
{
public:
    enum class ESelectionType : uint8
    {
        None,
        Component,
        PartObject,
    };

public:
    explicit Prefab_View();
    virtual ~Prefab_View();

public:
    void Initialize() override;
    void Update(float timeDelta) override;
    void OnGui() override;

    bool CanSave() const override;
    void Save() override;

public:
    void Open_Prefab(const string& prefabName, const string& prefabPath);
    void Close_Prefab();

    bool Is_Open() const { return _isOpen; }
    void Pre_Render() override;

    void Draw_PreviewCameraInspector();
    void Apply_PreviewCameraSettings();

private: 
    void Draw_Header();
    void Draw_ComponentList();
    void Draw_ComponentInspector();
    void Draw_Buttons();

    void Update_ImGuizmo();
    void Handle_Guizmo_Shotcut();
    void Update_PreviewCanvasState(const ImVec2& imagePos, const ImVec2& imageSize);

private:
    void            Preview_BeginPlay();
    Shared<Model>   Find_PreviewModel() const;


private: /* 애니메이션 */
    void    Draw_AnimationControls();
    void    Open_AnimationView();

private: /* 파츠 세팅 */
    void    Draw_PartObjectList();
    void    Draw_PartObjectInspector();

    Shared<ContainerObject> Get_PreviewContainer() const;
    Shared<Transform> Get_GizmoTargetTransform() const;

public:
    static shared_ptr<Prefab_View> Create();

private:
    bool    _isOpen = false;

    string  _prefabName;
    string  _prefabPath;

    Shared<GameObject>          _previewObject;
    Shared<Editor_Camera_Free>  _previewCamera;

    bool   _previewHasBegunPlay = false;

private: /* preview */
    Shared<RenderTarget>    _prevRT;
    Matrix                  _previewView;
    Matrix                  _previewProj;

    ImVec2                  _previewScreenPos;   
    ImVec2                  _previewViewportSize;

    Shared<Prefab_PreviewCameraSettings> _previewCameraSettings;

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;
    bool                _isPreviewHovered = false;
    bool                _isPreviewActive = false;
    bool                _isGizmoHovered = false;
    bool                _isGizmoUsing = false;

private: /* 노티파이 */
    int32   _previewSelectedAnimIndex = 0;
    bool    _previewAnimLoop = true;

private:
    ESelectionType _selectionType = ESelectionType::Component;

    uint32 _selectedComponentId = 0;
    uint32 _selectedPartSlot = 0;

};

NS_END
