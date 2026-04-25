#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class Shader;

class ENGINE_DLL Shadow : public Base
{
public:
    explicit Shadow() = default;
    virtual ~Shadow() = default;

public:
    // 현재 primary shadow light 설정을 받아 shadow view/proj 행렬을 갱신할 때 호출한다.
    HRESULT Update_LightDesc(const FLightDesc& lightDesc);
    // shadow pass 셰이더가 쓰는 view/proj 상수 이름에 맞춰 shadow 행렬을 바인딩할 때 호출한다.
    HRESULT Bind_TransformStateMatrix(Shared<Shader> shader, const char* viewName, const char* projName);
    // shadow를 끄거나 primary shadow light가 사라졌을 때 캐시된 상태를 비운다.
    void    Clear();

public:
    // 현재 shadow pass가 사용할 view 행렬이다.
    const Matrix* Get_ViewMatrix() const { return &_viewMatrix; }
    // 현재 shadow pass가 사용할 proj 행렬이다.
    const Matrix* Get_ProjMatrix() const { return &_projMatrix; }
    // deferred 합성 단계에서 shadow 튜닝 값을 읽기 위한 현재 light desc다.
    const FLightDesc* Get_LightDesc() const { return _hasLight ? &_lightDesc : nullptr; }
    // 이번 프레임 shadow light가 유효하게 세팅되었는지 확인할 때 사용한다.
    bool Has_Light() const { return _hasLight; }

public:
    static Unique<Shadow> Create();
    void Free() override;

private:
    // shadow 행렬 생성에 사용한 최신 light 설정 캐시다.
    FLightDesc   _lightDesc = {};
    // primary shadow light가 유효할 때만 true가 된다.
    bool         _hasLight = false;
    // shadow pass에서 모든 caster를 light 기준으로 보는 view 행렬이다.
    Matrix       _viewMatrix = Matrix::Identity;
    // shadow pass에서 사용하는 directional orthographic projection 행렬이다.
    // shadow pass에서 사용하는 projection 행렬이다. explicit shadow camera와 fallback 경로를 모두 담는다.
    Matrix       _projMatrix = Matrix::Identity;
};

NS_END
