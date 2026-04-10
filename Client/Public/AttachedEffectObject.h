#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Client)

class AttachedEffectObject final : public GameObject
{
    GENERATED_BODY(AttachedEffectObject)

public:
    struct FAttachedEffectObjectDesc : public GameObject::FGameObjectDesc
    {
        string effectAssetName = "";
        bool   loopOverride = false;
    };

public:
    explicit AttachedEffectObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit AttachedEffectObject(const AttachedEffectObject& rhs);
    virtual ~AttachedEffectObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

public:
    void Sync_AttachedTransform(
        const Matrix& boneWorldMatrix,
        const Vec3& localOffset,
        const Vec3& localRotation,
        const Vec3& localScale);

    void Stop_AttachedEffect();

    Shared<EffectComponent> Get_EffectComponent() const { return _effectCom; }

private:
    HRESULT Ready_Components();

private:
    Shared<EffectComponent> _effectCom;

    string _effectAssetName = "";
    bool   _loopOverride = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
