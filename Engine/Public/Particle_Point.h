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
        string maskTextureGuid;                     // Point 알파를 추가로 깎아낼 보조 마스크 텍스처 GUID다.
        string opacityTextureGuid;                  // Point 최종 alpha를 제어할 opacity 텍스처 GUID다.
        EEffectBlendMode blendMode = EEffectBlendMode::Additive; // Point 파티클 셰이더 패스 선택용 블렌드 모드다.
        Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f); // Point 텍스처를 원본 색 대신 원하는 색으로 틴트할 때 사용한다.
        float opacity = 1.f;                       // Point shader 최종 알파 강도 보정값이다.
        float emissiveStrength = 1.f;              // Billboard와 색감을 맞추기 위해 Point 발광 색을 증폭하는 계수다.
        FEffectFlipbookDesc flipbook;              // Point 텍스처를 Flipbook/SubUV 시트로 재생할 때 사용할 설정이다.
        Vec4 customParams0 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 0번 슬롯이다.
        Vec4 customParams1 = Vec4::Zero;           // 셰이더에서 자유롭게 읽을 수 있는 사용자 정의 파라미터 1번 슬롯이다.

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
    bool    Should_ExcludeFromEditorSnapshot() const override { return true; }
    HRESULT Bind_ShaderResources() override;
    // EffectComponent가 레이어 수명 보간 결과를 Point 오브젝트에 실시간 반영할 때 호출한다.
    void    Set_ColorTint(const Vec4& colorTint);
    // EffectComponent가 레이어 수명 보간 결과를 Point 알파 값으로 넘길 때 호출한다.
    void    Set_Opacity(float opacity);

private:
    // GUID 텍스처가 들어온 경우 에셋 경로를 해석해 Texture 컴포넌트를 동적으로 준비한다.
    HRESULT Resolve_TextureComponent(const FParticlePointDesc& desc);
    // mask / opacity처럼 선택 텍스처를 GUID 기반으로 준비할 때 공통 경로로 사용한다.
    HRESULT Resolve_OptionalTextureComponent(const string& textureGuid, Shared<Texture>& outTextureCom);
    // Point shader 안의 블렌드 pass 인덱스를 계산할 때 사용한다.
    uint32 Resolve_PassIndex() const;

private:
    HRESULT Ready_Components(const FParticlePointDesc& desc);

private:
    Shared<Shader>  _shaderCom;
    Shared<Texture> _textureCom;
    Shared<Texture> _maskTextureCom;                             // Point 알파를 추가로 조절할 마스크 텍스처다.
    Shared<Texture> _opacityTextureCom;                          // Point 최종 alpha 강도를 제어할 opacity 텍스처다.
    Shared<VIBuffer_Particle_Point> _bufferCom;

    uint32  _textureIndex = 0;
    bool    _addToBlendGroup = true;
    EEffectBlendMode _blendMode = EEffectBlendMode::Additive; // 현재 Point 오브젝트가 사용할 렌더 패스 상태다.
    Vec4 _colorTint = Vec4(1.f, 1.f, 1.f, 1.f);              // Point 텍스처를 마스크처럼 사용할 때 곱해질 최종 색상이다.
    float _opacity = 1.f;                                     // Point 텍스처의 최종 불투명도 강도다.
    float _emissiveStrength = 1.f;                            // Point 최종 발광 색상에 곱할 밝기 보정값이다.
    bool _useLifetimeFade = true;                             // 정적 데칼처럼 유지돼야 하는 Point인지, 수명 기반 fade를 쓸지 결정한다.
    FEffectFlipbookDesc _flipbook;                            // 현재 Point 텍스처를 어떤 Flipbook 설정으로 샘플링할지 기억한다.
    Vec4 _customParams0 = Vec4::Zero;                         // 현재 Point 셰이더에 바인딩할 사용자 정의 파라미터 0번 슬롯이다.
    Vec4 _customParams1 = Vec4::Zero;                         // 현재 Point 셰이더에 바인딩할 사용자 정의 파라미터 1번 슬롯이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
