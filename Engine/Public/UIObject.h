#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Shader;

#define PROPERTY_UIOBJECT_FORCE_VISIBLE() \
    PROPERTY_BOOL("Force Visible", _forceVisibleInspector)

class ENGINE_DLL UIObject abstract : public GameObject
{
    GENERATED_BODY(UIObject)

public:
    struct FUIDesc : public GameObject::FGameObjectDesc
    {
        // pos = 화면 중앙 기준
        float posX = {};
        float posY = {};
        float sizeX = {};
        float sizeY = {};
        float zOrder = 0.5f;

        uint32 levelIndex = 0;
    };

public:
    explicit UIObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UIObject(const UIObject& rhs);
    virtual ~UIObject();

public:
    virtual HRESULT     Initialize_Prototype() override;
    virtual HRESULT     Initialize(void* arg) override;

    virtual void        Priority_Update(float timeDelta) override;
    virtual void        Update(float timeDelta) override;
    virtual void        Late_Update(float timeDelta) override;
    virtual HRESULT     Render() override;

public:
    virtual void        Set_Visibility(bool active) { _isVisible = active; }
    virtual bool        Is_Visibility() const { return _isVisible; }
    bool                Is_VisibleForRender() const { return _isVisible || _forceVisibleInspector; }

    float               Get_ZOrder() const { return _zOrder; }
    EUILayer            Get_UILayer() const { return _uiLayer; }
    void                Set_UILayer(EUILayer layer) { _uiLayer = layer; };

    uint32              Get_LevelIndex() const { return _levelIndex; }
    void                Set_LevelIndex(uint32 index) { _levelIndex = index; }

public: /* UI Animation Track*/
    void    Set_UIPosition(float x, float y);
    void    Set_UIScale(float x, float y);
    void    Set_UIRotationZ(float degree);
    void    Set_UIOpacity(float alpha);
    void    Set_UITint(const Color& color);

    float   Get_UIPosX() const { return _posX; }
    float   Get_UIPosY() const { return _posY; }
    float   Get_UISizeX() const { return _sizeX; }
    float   Get_UISizeY() const { return _sizeY; }
    float   Get_UIRotationZ() const { return _rotationZ; }
    float   Get_UIOpacity() const { return _opacity; }
    const Color& Get_UITint() const { return _tintColor; }

    virtual HRESULT     On_UIRegistered() { return S_OK; }

public:
    ERenderGroup Get_RenderGroup() const { return _renderGroup; }
    void Set_RenderGroup(ERenderGroup group) { _renderGroup = group; }

protected:
    // SRT로 월드 행렬 갱신, 직교 투영 행렬 세팅
    void    Update_Transform();

    // 셰이더에 변환 행렬 바인딩용
    HRESULT Bind_ShaderResource(Shared<Shader> shader, const char* constantName, ETransformState transformState);

    virtual HRESULT     Ready_Components() { return S_OK; }

protected:
    float _posX{}, _posY{}, _sizeX{}, _sizeY{};
    float _zOrder{};

    // 뷰포트 크기 (직교투영 계산용)
    float _viewportWidth{}, _viewportHeight{};

    // UI 전용 변환 행렬
    Matrix _worldMatrix = Matrix::Identity;
    Matrix _transformMatrices[ETOI(ETransformState::END)];

protected:
    bool        _isVisible  = true; // 화면 표시
    bool        _forceVisibleInspector = false; // 인스펙터에서 UI 애니메이션 편집 시 숨김 상태를 렌더링만 강제로 우회
    EUILayer    _uiLayer    = EUILayer::HUD;

    uint32      _levelIndex = 0;

    ERenderGroup _renderGroup = ERenderGroup::UI;

protected:
    float       _rotationZ = 0.f;
    float       _opacity = 1.f;
    Color       _tintColor = Color(1.f, 1.f, 1.f, 1.f);

public:
    virtual Shared<GameObject> Clone(void* arg) { return nullptr; }
    virtual void Free() override;

};

NS_END
