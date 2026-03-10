#pragma once

#include "EditorWindow.h"

NS_BEGIN(Engine)
class GameObject;
class RenderTarget;
NS_END

NS_BEGIN(Editor)

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

private: 
    void Draw_Header();
    void Draw_ComponentList();
    void Draw_ComponentInspector();
    void Draw_Buttons();

    void Update_ImGuizmo();
    void Handle_Guizmo_Shotcut();

public:
    static shared_ptr<Prefab_View> Create();

private:
    bool    _isOpen = false;

    string  _prefabName;
    string  _prefabPath;

    Shared<GameObject>          _previewObject;
    Shared<Editor_Camera_Free>  _previewCamera;

    uint32 _selectedComponentId = 0;

private: /* preview */
    Shared<RenderTarget>    _prevRT;
    Matrix                  _previewView;
    Matrix                  _previewProj;

    ImVec2                  _previewScreenPos;   
    ImVec2                  _previewImGuiSize;

    //Unique<PrimitiveBatch<DirectX::VertexPositionColor>> _gridBatch = nullptr;
    //Unique<BasicEffect> _gridEffect = nullptr;
    //ComPtr<ID3D11InputLayout>                        _gridInputLayout;

private: /* ImGuizmo */
    ImGuizmo::OPERATION _gizmoOperation = ImGuizmo::TRANSLATE;

};

NS_END
