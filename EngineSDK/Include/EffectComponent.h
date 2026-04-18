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

        bool  worldLocked = false;
        Vec3  lockedWorldPos = Vec3::Zero;
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
    // 현재 이 컴포넌트가 마지막으로 재생 요청받은 이펙트 에셋 이름을 디버그/인스펙터에서 확인할 때 호출한다.
    const string& Get_CurrentAssetName() const { return _assetName; }
    // 현재 재생 중인 이펙트 에셋 원본 데이터를 인스펙터에서 레이어별로 펼쳐볼 때 호출한다.
    const FEffectAssetDesc& Get_CurrentAsset() const { return _asset; }
    // 런타임에 생성된 활성 레이어 상태를 인스펙터에서 읽어올 때 호출한다.
    const vector<FActiveLayer>& Get_ActiveLayers() const { return _layers; }
    // 현재 이펙트가 재생된 뒤 누적된 시간을 디버그 표시할 때 호출한다.
    float Get_LifeSpan() const { return _lifeSpan; }

    bool    Apply_LayerDesc(int32 layerIndex, const FEffectLayerDesc& layerDesc, bool rebuildObject);
    bool    Apply_LayerTransform(int32 layerIndex, const FEffectLayerBase& baseDesc);

    void    Set_ForceVisiblePreview(bool enabled);

    // NotifyState에서 조절
    void    Set_RuntimeLocalTransform(const Vec3& localPosition, const Vec3& localRotation, const Vec3& localScale);
    void    Clear_RuntimeLocalTransform();

private:
    HRESULT Create_LayerObject(FActiveLayer& layer);

    void    Apply_LayerTransformInternal(FActiveLayer& layer);
    // 레이어 Position Over Time 계산 결과를 현재 오브젝트 로컬 위치에만 반영할 때 호출한다.
    void    Apply_LayerPositionInternal(FActiveLayer& layer);
    void    Apply_LayerScaleInternal(FActiveLayer& layer);
    // 레이어 수명에 따라 tint/opacity/emissive 값을 계산해 현재 오브젝트에 반영할 때 호출한다.
    void    Apply_LayerAnimatedMaterialInternal(FActiveLayer& layer);
    // Position Over Time이 켜진 레이어의 현재 로컬 위치를 계산할 때 호출한다.
    Vec3    Resolve_LayerPosition(const FActiveLayer& layer) const;
    Vec3    Resolve_LayerScale(const FActiveLayer& layer) const;
    float   Resolve_LayerDuration(const FActiveLayer& layer) const;
    // 현재 레이어가 자신의 수명 중 어디까지 진행됐는지 0~1로 계산할 때 호출한다.
    float   Resolve_LayerProgress(const FActiveLayer& layer) const;

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
