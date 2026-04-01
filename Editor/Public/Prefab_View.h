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
    // 프리팹 프리뷰 RT에 사용할 디버그 그리드 리소스를 필요 시점에만 준비한다.
    void Ensure_PreviewGridResources();
    // 프리뷰 모델의 바닥 기준선을 보기 쉽게 하기 위해 RT에 그리드를 그린다.
    void Draw_PreviewGrid() const;

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
    // 프리팹 프리뷰 RT에 라인 기반 그리드를 그릴 때 사용하는 배치 객체다.
    Shared<DirectX::PrimitiveBatch<DirectX::VertexPositionColor>> _previewGridBatch;
    // 프리팹 프리뷰 그리드용 정점 컬러 이펙트다.
    Shared<DirectX::BasicEffect> _previewGridEffect;
    // 프리팹 프리뷰 그리드 렌더링 시 VertexPositionColor 포맷을 맞추기 위한 입력 레이아웃이다.
    ComPtr<ID3D11InputLayout> _previewGridInputLayout;
    // 프리팹 프리뷰 그리드를 항상 보이게 그리기 위한 depth off 상태다.
    ComPtr<ID3D11DepthStencilState> _previewGridDepthDisabledState;

    ImVec2                  _previewScreenPos;   
    ImVec2                  _previewViewportSize;
    // 프리팹 프리뷰 그리드 표시 여부를 제어한다.
    bool                    _showPreviewGrid = true;

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
