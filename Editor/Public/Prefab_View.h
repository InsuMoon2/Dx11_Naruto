#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
class RenderTarget;
class Model;
NS_END

NS_BEGIN(Editor)
    class Prefab_PreviewCameraSettings;
    class Editor_Camera_Free;

class Prefab_View : public EditorWindow
{
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

private:
    void            Preview_BeginPlay();
    Shared<Model>   Find_PreviewModel() const;
    void            Tick_PreviewAnimation(float timeDelta);

private:
    void    Draw_AnimationControls();
    void    Open_AnimationView();

public:
    static shared_ptr<Prefab_View> Create();

private:
    bool    _isOpen = false;

    string  _prefabName;
    string  _prefabPath;

    Shared<GameObject>          _previewObject;
    Shared<Editor_Camera_Free>  _previewCamera;

    uint32 _selectedComponentId = 0;

    bool   _previewHasBegunPlay = false;

private: /* preview */
    Shared<RenderTarget>    _prevRT;
    Matrix                  _previewView;
    Matrix                  _previewProj;

    ImVec2                  _previewScreenPos;   
    ImVec2                  _previewImGuiSize;

    Shared<Prefab_PreviewCameraSettings> _previewCameraSettings;

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;

private: /* 노티파이 */
    int32   _previewSelectedAnimIndex = 0;
    bool    _previewAnimLoop = true;

};

NS_END
