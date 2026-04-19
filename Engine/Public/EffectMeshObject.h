#pragma once

#include "GameObject.h"
#include "EffectAsset_Types.h"

NS_BEGIN(Engine)

class Shader;
class Model;
class Texture;

class ENGINE_DLL EffectMeshObject : public GameObject
{
public:
    struct FEffectMeshDesc : public FGameObjectDesc
    {
        FEffectLayerDesc layerDesc;
    };

    struct FEffectMeshMaterialRuntimeDesc
    {
        string diffuseTextureGuid;
        string maskTextureGuid;
        string emissiveTextureGuid;
        string opacityTextureGuid;
        string opacitySubUvTextureGuid;
        string opacityGradationTextureGuid;
        string emissiveGradationTextureGuid;
        string uvDistortionTextureGuid;
        string normalTextureGuid;
        string roughnessTextureGuid;
        string specularTextureGuid;

        EEffectBlendMode blendMode = EEffectBlendMode::Translucent;
        EEffectMeshShadingMode shadingMode = EEffectMeshShadingMode::Unlit;

        Vec2 uvScrollSpeed = Vec2(0.f, 0.f);
        Vec2 uvTiling = Vec2(1.f, 1.f);
        Vec2 uvDistortionStrength = Vec2(0.f, 0.f);
        Vec2 uvDistortionSpeed = Vec2(0.f, 0.f);
        FEffectFlipbookDesc flipbook;

        Vec4 colorTint = Vec4(1.f, 1.f, 1.f, 1.f);
        float opacity = 1.f;
        float normalStrength = 1.f;
        float roughness = 0.5f;
        float specularStrength = 1.f;
        float specularPower = 32.f;
        float emissiveStrength = 1.f;
        float fresnelPower = 0.f;
        float fresnelMultiplier = 1.f;
        Vec4 customParams0 = Vec4::Zero; // 장벽/림 같은 Mesh 전용 셰이더 옵션을 런타임에 넘길 사용자 정의 파라미터 0번 슬롯이다.
        Vec4 customParams1 = Vec4::Zero; // 장벽/림 같은 Mesh 전용 셰이더 옵션을 런타임에 넘길 사용자 정의 파라미터 1번 슬롯이다.

        bool twoSided = false;
        bool useOpacityAsTransparency = false;
    };

    struct FResolvedMaterialResources
    {
        string materialName;
        Shared<Texture> diffuseTexture;
        Shared<Texture> maskTexture;
        Shared<Texture> emissiveTexture;
        Shared<Texture> opacityTexture;
        Shared<Texture> opacitySubUvTexture;
        Shared<Texture> opacityGradationTexture;
        Shared<Texture> emissiveGradationTexture;
        Shared<Texture> uvDistortionTexture;
        Shared<Texture> normalTexture;
        Shared<Texture> roughnessTexture;
        Shared<Texture> specularTexture;
    };

public:
    explicit EffectMeshObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectMeshObject(const EffectMeshObject& rhs);
    virtual ~EffectMeshObject() = default;

public:
    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;
    virtual void    Priority_Update(float timeDelta) override;
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;
    virtual bool    Should_ExcludeFromEditorSnapshot() const override { return true; }

    virtual HRESULT Bind_ShaderResources() override;
    HRESULT Bind_ShaderResources(uint32 meshIndex);

    HRESULT Resolve_Resources();

public:
    void  Set_ElapsedTime(float time) { _elapsed = time; }
    float Get_ElapsedTime() const { return _elapsed; }

    void Apply_LayerDesc(const FEffectLayerDesc& layerDesc);
    void Apply_BaseTransform(const FEffectLayerBase& baseDesc);
    void Set_ForceVisiblePreview(bool enabled) { _forceVisiblePreview = enabled; }
    void Set_RuntimeColorTintOverride(const Vec4& colorTint, bool enabled);
    void Set_RuntimeOpacityOverride(float opacity, bool enabled);
    void Set_RuntimeEmissiveStrengthOverride(float emissiveStrength, bool enabled);

    // EffectComponent가 오너/런타임 오프셋까지 포함한 현재 로컬 회전을 "회전 기준 자세"로 다시 잡아야 할 때 호출한다.
    void Sync_RotationBaseFromCurrentTransform();
    void Update_Rotation(float timeDelta);

    const FEffectLayerDesc& Get_LayerDesc() const { return _layerDesc; }

private:
    HRESULT Ready_Components();
    HRESULT Apply_AnimationSettings();
    HRESULT Resolve_TextureByGuid(const string& guid, Shared<Texture>& outTexture);
    HRESULT Resolve_OverrideResources();

    uint32 Resolve_PassIndex() const;
    uint32 Resolve_PassIndex(const FEffectMeshMaterialRuntimeDesc& meshDesc) const;
    bool   Is_SkeletalLayer() const;
    uint32 Resolve_ShaderComponentId() const;
    bool   Has_OpacityTexture(const FEffectMeshMaterialRuntimeDesc& meshDesc) const;
    bool   Has_NonOpaquePass() const;
    FEffectMeshMaterialRuntimeDesc Resolve_RuntimeMeshDesc(uint32 meshIndex) const;
    const FResolvedMaterialResources* Resolve_RuntimeMaterialResources(uint32 meshIndex) const;

private:
    Shared<Shader>  _shaderCom;
    Shared<Model>   _modelCom;
    Shared<Texture> _diffuseTexture;
    Shared<Texture> _maskTexture;
    Shared<Texture> _emissiveTexture;
    Shared<Texture> _opacityTexture;
    Shared<Texture> _opacitySubUvTexture;
    Shared<Texture> _opacityGradationTexture;
    Shared<Texture> _emissiveGradationTexture;
    Shared<Texture> _uvDistortionTexture;
    Shared<Texture> _normalTexture;            // Lit 모드에서 노멀맵 샘플링에 사용할 텍스처 컴포넌트다.
    Shared<Texture> _roughnessTexture;         // Lit 모드에서 러프니스 값을 읽어올 텍스처 컴포넌트다.
    Shared<Texture> _specularTexture;          // Lit 모드에서 스페큘러 마스크를 읽어올 텍스처 컴포넌트다.
    vector<FResolvedMaterialResources> _materialOverrideResources;

    FEffectLayerDesc _layerDesc;

    float _elapsed = 0.f;                     
    float _accumulatedRotation = 0.f;         // 누적 회전 각도(radian)다. 매 프레임 delta를 곱적하지 않고 안정적으로 기준 자세에서 다시 계산할 때 쓴다.
    bool  _hasOpacity = false;                
    Quat  _rotationBaseLocal = Quat::Identity; // Local Rotation + 오너 오프셋까지 반영된 "회전 시작 자세"를 기억해 owner/local 축 회전을 안정적으로 재구성한다.
    // 이펙트 뷰 진단용으로만 쓰는 강제 가시화 플래그다. 런타임 기본 동작은 false를 유지한다.
    bool  _forceVisiblePreview = false;
    bool  _useRuntimeColorTintOverride = false;        // true면 layerDesc의 기본 tint 대신 런타임 보간 tint를 셰이더에 바인딩한다.
    Vec4  _runtimeColorTint = Vec4(1.f, 1.f, 1.f, 1.f); // 레이어 수명 보간으로 계산된 현재 Mesh tint 값이다.
    bool  _useRuntimeOpacityOverride = false;          // true면 layerDesc의 기본 opacity 대신 런타임 보간 opacity를 셰이더에 바인딩한다.
    float _runtimeOpacity = 1.f;                       // 레이어 수명 보간으로 계산된 현재 Mesh opacity 값이다.
    bool  _useRuntimeEmissiveStrengthOverride = false; // true면 layerDesc의 기본 emissiveStrength 대신 런타임 보간 값을 바인딩한다.
    float _runtimeEmissiveStrength = 1.f;              // 레이어 수명 보간으로 계산된 현재 Mesh emissive 강도다.
    Vec4  _runtimeCustomParams0 = Vec4::Zero;          // 현재 Mesh 셰이더에 바인딩할 사용자 정의 파라미터 0번 슬롯이다.
    Vec4  _runtimeCustomParams1 = Vec4::Zero;          // 현재 Mesh 셰이더에 바인딩할 사용자 정의 파라미터 1번 슬롯이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;
};

NS_END
