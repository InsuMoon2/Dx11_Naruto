#pragma once
#include "Component.h"
#include "EffectAsset_Types.h"
#include "EffectMeshObject.h"

NS_BEGIN(Engine)

class ENGINE_DLL EffectComponent : public Component
{
    GENERATED_COMPONENT(EffectComponent, Protocol::COMPONENT_TYPE_EFFECT)

public:
    struct FPlayDesc
    {
        string  effectAssetName;
        Vec3    localPosition = Vec3::Zero;
        Vec3    localRotation = Vec3::Zero;
        Vec3    localScale = Vec3(1.f, 1.f, 1.f);
        bool    loopOverride = false;
    };

    struct FActiveLayer
    {
        FEffectLayerDesc desc;
        Shared<GameObject> obj;

        float elapsed = 0.f;
        bool  started = false, finished = false;
    };

public:
    explicit EffectComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit EffectComponent(const EffectComponent& rhs);
    virtual ~EffectComponent() = default;

    virtual HRESULT Initialize_Prototype() override;
    virtual HRESULT Initialize(void* arg) override;

    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);

    HRESULT Play_Effect(const FPlayDesc& desc);

    // 실시간 에디터 동기화용
    HRESULT Play_EffectAsset(const FEffectAssetDesc& assetDesc);

    void    Stop_Effect();
    bool    Is_Playing() const { return _isPlaying; }

    bool    Apply_LayerDesc(int32 layerIndex, const FEffectLayerDesc& layerDesc, bool rebuildObject);
    bool    Apply_LayerTransform(int32 layerIndex, const FEffectLayerBase& baseDesc);

    void    Set_ForceVisiblePreview(bool enabled);

    // NotifyState에서 조절
    void    Set_RuntimeLocalTransform(const Vec3& localPosition, const Vec3& localRotation, const Vec3& localScale);
    void    Clear_RuntimeLocalTransform();

private:
    HRESULT Create_LayerObject(FActiveLayer& layer);

    void    Apply_LayerTransformInternal(FActiveLayer& layer);
    void    Apply_LayerScaleInternal(FActiveLayer& layer);
    Vec3    Resolve_LayerScale(const FActiveLayer& layer) const;
    float   Resolve_LayerDuration(const FActiveLayer& layer) const;

    string  Resolve_EffectAssetPathByName(const string& effectAssetName);

private:
    string                  _assetName;
    FEffectAssetDesc        _asset;
    vector<FActiveLayer>    _layers;

    float                   _lifeSpan = 0.f;
    bool                    _isPlaying = false;

    bool                    _forceVisiblePreview = false;

    FPlayDesc               _desc;

private:
    bool    _useRuntimeLocalTransform = false;
    Vec3    _runtimeLocalPosition = Vec3::Zero;
    Vec3    _runtimeLocalRotation = Vec3::Zero;
    Vec3    _runtimeLocalScale = Vec3::One;

public:
    static Shared<EffectComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};


NS_END
