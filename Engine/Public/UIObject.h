#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)

class Shader;

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
    void                Set_Active(bool bActive) { _isActive = bActive; }
    bool                Is_Active() const { return _isActive; }

    float               Get_ZOrder() const { return _zOrder; }
    EUILayer            Get_UILayer() const { return _uiLayer; }
    void                Set_UILayer(EUILayer layer) { _uiLayer = layer; };

    uint32              Get_LevelIndex() const { return _levelIndex; }
    void                Set_LevelIndex(uint32 index) { _levelIndex = index; }

public:
    template<typename T>
    Shared<T> Create_Child(EUILayer uiLayer, void* arg)
    {
        Shared<T> childUI = T::Create(_device, _context, arg);
        if (!childUI) return nullptr;

        // 부모 레벨 세팅
        childUI->Set_LevelIndex(this->_levelIndex);
        childUI->Set_UILayer(uiLayer);

        // 자식 트랜스폼 세팅
        childUI->Get_Transform()->Set_Parent(this->Get_Transform());

        GAME->Add_UI_ToLayer(uiLayer, childUI);
    }

protected:
    // SRT로 월드 행렬 갱신, 직교 투영 행렬 세팅
    void    Update_Transform();

    // 셰이더에 변환 행렬 바인딩용
    HRESULT Bind_ShaderResource(Shared<Shader> shader, const char* constantName, ETransformState transformState);

    virtual HRESULT     Ready_Components() {};

protected:
    float _posX{}, _posY{}, _sizeX{}, _sizeY{};
    float _zOrder{};

    // 뷰포트 크기 (직교투영 계산용)
    float _viewportWidth{}, _viewportHeight{};

    // UI 전용 변환 행렬
    Matrix _worldMatrix = Matrix::Identity;
    Matrix _transformMatrices[ETOI(ETransformState::END)];

protected:
    bool        _isActive = true; // 화면 표시
    EUILayer    _uiLayer = EUILayer::HUD;

    uint32      _levelIndex = 0;

public:
    virtual Shared<GameObject> Clone(void* arg) { return nullptr; }
    virtual void Free() override;

};

NS_END
