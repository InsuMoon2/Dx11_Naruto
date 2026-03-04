#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
class RenderTarget;
NS_END

NS_BEGIN(Editor)

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

private:
    void Draw_Header();
    void Draw_ComponentList();
    void Draw_Buttons();

    void Update_ImGuizmo();
    void Handle_Guizmo_Shotcut();

public:
    static shared_ptr<Prefab_View> Create();

private:
    bool _isOpen = false;

    string _prefabName;
    string _prefabPath;

    Shared<GameObject> _targetObject;

private: /* preview */
    Shared<RenderTarget>    _prevRT;
    Matrix                  _previewView;
    Matrix                  _previewProj;
    Vec3                    _previewCamPos;
    float                   _previewYaw = 0;
    float                   _previewPitch = 1.614;
    float                   _previewDistnace = 5.f;

    ImVec2                  _previewScreenPos;   
    ImVec2                  _previewImGuiSize;

    Vec3                    _previewCenter = { 6.3f, 0.5f, 4.8f };

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;

};

NS_END
