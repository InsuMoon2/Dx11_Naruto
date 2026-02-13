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
    };

public:
    explicit UIObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UIObject(const UIObject& rhs);
    virtual ~UIObject();

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;

    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

protected:
    // SRT로 월드 행렬 갱신, 직교 투영 행렬 세팅
    void    Update_Transform();

    // 셰이더에 변환 행렬 바인딩용
    HRESULT Bind_ShaderResource(Shared<Shader> shader, const char* constantName, EUITransformState transformState);

protected:
    float _posX{}, _posY{}, _sizeX{}, _sizeY{};

    // 뷰포트 크기 (직교투영 계산용)
    float _viewportWidth{}, _viewportHeight{};

    // UI 전용 변환 행렬
    Matrix _worldMatrix = Matrix::Identity;
    Matrix _transformMatrices[ETOI(EUITransformState::END)];

public:
    virtual Shared<GameObject> Clone(void* arg) abstract;
    virtual void Free() override;

};

NS_END
