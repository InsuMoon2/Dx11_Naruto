#pragma once

#include "GameObject.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class Shader;
class Texture;
class VIBuffer_Rect;

class ENGINE_DLL EffectBillboardObject : public GameObject
{
    GENERATED_BODY(EffectBillboardObject)

public:
    struct FEffectBillboardDesc : public GameObject::FGameObjectDesc
    {
        FEffectLayerDesc layerDesc; 
    };

public:
    explicit EffectBillboardObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectBillboardObject(const EffectBillboardObject& rhs);
    virtual ~EffectBillboardObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;
    bool    Should_ExcludeFromEditorSnapshot() const override { return true; }
    HRESULT Bind_ShaderResources() override;

public:
    void Apply_LayerDesc(const FEffectLayerDesc& layerDesc);
    // EffectComponent가 레이어 루프/지속시간과 Flipbook 시간을 정확히 동기화할 때 호출한다.
    void Set_ElapsedTime(float elapsedTime) { _elapsedTime = elapsedTime; }
    // EffectComponent가 레이어 수명 보간 결과를 중심 레이어 opacity에 덮어쓸 때 호출한다.
    void Set_RuntimeBaseOpacityOverride(float baseOpacity, bool enabled);
    // EffectComponent가 레이어 수명 보간 결과를 링 레이어 opacity에 덮어쓸 때 호출한다.
    void Set_RuntimeRingOpacityOverride(float ringOpacity, bool enabled);

private:
    void Update_BillboardRotation();
    HRESULT Resolve_Textures();
    HRESULT Resolve_TextureComponent(const string& textureGuid, Shared<Texture>& outTextureCom);
    uint32 Resolve_PassIndex() const;
    void Apply_BaseTransform(const FEffectLayerBase& baseDesc);

private:
    Shared<Shader>        _shaderCom;
    Shared<VIBuffer_Rect> _bufferCom;
    Shared<Texture>       _baseTextureCom;                  // Billboard 중심/데칼 색 텍스처를 렌더링할 때 사용하는 기본 텍스처 컴포넌트다.
    Shared<Texture>       _baseMaskTextureCom;              // FlipbookDecal Billboard가 사각 프레임 배경을 잘라낼 때 사용할 마스크 텍스처 컴포넌트다.
    Shared<Texture>       _baseOpacityTextureCom;           // Billboard 중심 알파를 원본 opacity map처럼 별도 제어할 때 사용하는 텍스처 컴포넌트다.
    Shared<Texture>       _baseOpacityGradationTextureCom;  // Billboard 중심 opacity 값을 GMO 계열 텍스처로 리매핑할 때 사용하는 텍스처 컴포넌트다.
    Shared<Texture>       _ringTextureCom;                  // Billboard 외곽 보조 링을 렌더링할 때 사용하는 텍스처 컴포넌트다.
    Shared<Texture>       _ringOpacityTextureCom;           // Billboard 링 알파를 원본 opacity map처럼 별도 제어할 때 사용하는 텍스처 컴포넌트다.
    Shared<Texture>       _ringOpacityGradationTextureCom;  // Billboard 링 opacity 값을 GMO 계열 텍스처로 리매핑할 때 사용하는 텍스처 컴포넌트다.
    Shared<Texture>       _screenDistortionNormalTextureCom; // ScreenDistortion 모드에서 SceneColorCopy UV를 흔드는 노멀/노이즈 텍스처 컴포넌트다.

    FEffectLayerDesc      _layerDesc{}; 
    string                _resolvedBaseTextureGuid;         // 현재 _baseTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedBaseMaskTextureGuid;     // 현재 _baseMaskTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedBaseOpacityTextureGuid;  // 현재 _baseOpacityTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedBaseOpacityGradationTextureGuid; // 현재 _baseOpacityGradationTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedRingTextureGuid;         // 현재 _ringTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedRingOpacityTextureGuid;  // 현재 _ringOpacityTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedRingOpacityGradationTextureGuid; // 현재 _ringOpacityGradationTextureCom이 어떤 GUID를 가리키는지 기억해서 불필요한 재로딩을 막는다.
    string                _resolvedScreenDistortionNormalTextureGuid; // 현재 _screenDistortionNormalTextureCom이 어떤 GUID를 가리키는지 기억한다.
    bool                  _useRuntimeBaseOpacityOverride = false; // true면 baseOpacity 대신 런타임 보간 opacity를 셰이더에 바인딩한다.
    float                 _runtimeBaseOpacity = 1.f;              // 레이어 수명 보간으로 계산된 현재 중심 opacity 값이다.
    bool                  _useRuntimeRingOpacityOverride = false; // true면 ringOpacity 대신 런타임 보간 opacity를 셰이더에 바인딩한다.
    float                 _runtimeRingOpacity = 1.f;              // 레이어 수명 보간으로 계산된 현재 링 opacity 값이다.
    float                 _elapsedTime = 0.f;                     // Billboard Flipbook이 현재 몇 초까지 진행됐는지 기억하는 시간값이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
