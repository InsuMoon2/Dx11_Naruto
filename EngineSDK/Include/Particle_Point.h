#pragma once

#include "GameObject.h"
#include "EffectAsset_Types.h"
#include "VIBuffer_Particle_Point.h"

NS_BEGIN(Engine)
class Shader;
class Texture;

enum class EParticleTextureType
{
    Hit, 
};

class ENGINE_DLL Particle_Point : public GameObject
{
    GENERATED_BODY(Particle_Point)

public:
    struct FParticlePointDesc : public GameObject::FGameObjectDesc
    {
        uint32 shaderType = Protocol::COMPONENT_TYPE_SHADER_PARTICLE_POINT;
        uint32 textureType = Protocol::COMPONENT_TYPE_TEXTURE_PARTICLE_SNOW;
        string textureGuid;                         // 이펙트 툴 경로에서는 GUID 기반 텍스처를 직접 지정할 때 사용한다.
        EEffectBlendMode blendMode = EEffectBlendMode::Additive; // Point 파티클 셰이더 패스 선택용 블렌드 모드다.
        Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); // Point 텍스처를 원본 색 대신 원하는 색으로 틴트할 때 사용한다.
        float opacity = 1.f;                       // Point shader 최종 알파 강도 보정값이다.

        uint32 textureIndex = 0;

        VIBuffer_Particle_Point::FParticlePointDesc bufferDesc{};

        bool addToBlendGroup = false;              // Opaque가 아닌 경우 Blend 렌더 그룹으로 보내기 위한 플래그다.
    };

public:
    explicit Particle_Point(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Particle_Point(const Particle_Point& rhs);
    virtual ~Particle_Point() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;
    HRESULT Bind_ShaderResources() override;

private:
    // GUID 텍스처가 들어온 경우 에셋 경로를 해석해 Texture 컴포넌트를 동적으로 준비한다.
    HRESULT Resolve_TextureComponent(const FParticlePointDesc& desc);
    // Point shader 안의 블렌드 pass 인덱스를 계산할 때 사용한다.
    uint32 Resolve_PassIndex() const;

private:
    HRESULT Ready_Components(const FParticlePointDesc& desc);

private:
    Shared<Shader>  _shaderCom;
    Shared<Texture> _textureCom;
    Shared<VIBuffer_Particle_Point> _bufferCom;

    uint32  _textureIndex = 0;
    bool    _addToBlendGroup = true;
    EEffectBlendMode _blendMode = EEffectBlendMode::Additive; // 현재 Point 오브젝트가 사용할 렌더 패스 상태다.
    Vec4 _colorTint = Vec4(1.f, 1.f, 1.f, 1.f);              // Point 텍스처를 마스크처럼 사용할 때 곱해질 최종 색상이다.
    float _opacity = 1.f;                                     // Point 텍스처의 최종 불투명도 강도다.
    bool _useLifetimeFade = true;                             // 정적 데칼처럼 유지돼야 하는 Point인지, 수명 기반 fade를 쓸지 결정한다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
