#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Shader;
class Model;
NS_END

NS_BEGIN(Client)

class SkySphereActor final : public GameObject
{
     GENERATED_BODY(SkySphereActor)

public:
     struct FSkySphereDesc : public FGameObjectDesc
     {
         // 어떤 모델 프로토타입을 붙일지 결정하는 DT_Model ID 문자열이다.
         string modelComponentName;

         // 카메라를 따라 움직여 sky가 항상 멀리 있는 것처럼 보이게 한다.
         bool followCamera = true;
         // 투명 알파 블렌딩 경로로 보낼지 여부다.
         bool useBlend = false;
         // 안쪽에서도 보이도록 양면 렌더링을 사용할지 여부다.
         bool twoSided = false;

         // Sky shader가 어떤 스타일로 그릴지 결정하는 모드다. 0=기본, 1=BaseSky, 2=CloudLayer.
         int renderStyle = 0;

         // 메인 UV의 타일링 배율이다.
         Vec2 uvTiling = Vec2(1.f, 1.f);
         // 메인 UV의 스크롤 속도다.
         Vec2 uvScrollSpeed = Vec2::Zero;
         // 전체 색 보정용 메인 틴트다.
         Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f);
         // BaseSky의 지평선 색이다.
         Vec4 horizonColor = Vec4(1.f, 1.f, 1.f, 1.f);
         // BaseSky의 천정 색이다.
         Vec4 zenithColor = Vec4(1.f, 1.f, 1.f, 1.f);

         // CloudLayer의 보조 UV 타일링 배율이다.
         Vec2 subUVTiling = Vec2(1.f, 1.f);
         // CloudLayer의 보조 UV 스크롤 속도다.
         Vec2 subUVScrollSpeed = Vec2::Zero;

         // 최종 알파 강도다.
         float opacity = 1.f;
         // 최종 발광 강도다.
         float emissiveStrength = 1.f;
     };

public:
    explicit SkySphereActor(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkySphereActor(const SkySphereActor& rhs);
    virtual ~SkySphereActor() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    HRESULT Render() override;
    HRESULT Bind_ShaderResources() override;

private:
    HRESULT Ready_Components();
    uint32  Resolve_PassIndex() const;

private:
    Shared<Shader> _shaderCom; 
    Shared<Model> _modelCom; 

    string _modelComponentName;
    uint32 _modelComponentId = 0;

    bool _followCamera = true;
    bool _useBlend = false;
    bool _twoSided = false;

    // Sky shader가 어떤 방식으로 색과 알파를 계산할지 결정하는 렌더 모드다.
    int _renderStyle = 0;

    Vec2 _uvTiling = Vec2(1.f, 1.f);
    Vec2 _uvScrollSpeed = Vec2::Zero;
    Vec4 _colorTint = Vec4(1.f, 1.f, 1.f, 1.f);
    // BaseSky에서 지평선 쪽으로 사용할 색이다.
    Vec4 _horizonColor = Vec4(1.f, 1.f, 1.f, 1.f);
    // BaseSky에서 하늘 꼭대기 쪽으로 사용할 색이다.
    Vec4 _zenithColor = Vec4(1.f, 1.f, 1.f, 1.f);
    // CloudLayer의 보조 UV 타일링 배율이다.
    Vec2 _subUVTiling = Vec2(1.f, 1.f);
    // CloudLayer의 보조 UV 스크롤 속도다.
    Vec2 _subUVScrollSpeed = Vec2::Zero;

    float _opacity = 1.f;
    float _emissiveStrength = 1.f;
    // UV panner 계산에 사용하는 누적 시간이다.
    float _elapsedTime = 0.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
