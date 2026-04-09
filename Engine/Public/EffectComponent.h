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
        string  effectAssetGuid;
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
    // 이펙트 뷰 프리뷰에서 메시를 강제로 보이게 해야 할 때 현재/이후 레이어 오브젝트에 동일하게 적용한다.
    void    Set_ForceVisiblePreview(bool enabled);

private:
    HRESULT Create_LayerObject(FActiveLayer& layer);

    void    Apply_LayerTransformInternal(FActiveLayer& layer);

private:
    string                  _assetGuid;
    FEffectAssetDesc        _asset;
    vector<FActiveLayer>    _layers;

    float                   _lifeSpan = 0.f;
    bool                    _isPlaying = false;
    // 런타임은 false, 에디터 프리뷰는 true로 켜서 effect shader 출력이 0이어도 메시 실루엣을 확인한다.
    bool                    _forceVisiblePreview = false;

    FPlayDesc               _desc;

public:
    static Shared<EffectComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual Shared<Component> Clone(void* arg) override;
    virtual void Free() override;
};


NS_END
