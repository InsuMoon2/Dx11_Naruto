#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Client)

class MeshDebrisObject : public GameObject
{
    GENERATED_BODY(MeshDebrisObject)

public:
    struct FMeshDebrisDesc : public FGameObjectDesc
    {
        string  effectAssetName;
        Vec3    spawnRotation = Vec3::Zero;
        Vec3    spawnScale = Vec3(1.f, 1.f, 1.f);
        Vec3    effectLocalPosition = Vec3::Zero;
        Vec3    effectLocalRotation = Vec3::Zero;
        Vec3    effectLocalScale = Vec3(1.f, 1.f, 1.f);
        Vec3    initialVelocity = Vec3::Zero;

        float   gravity = -20.f;

        Vec3    angularVelocityDeg = Vec3::Zero;

        float   lifetime = 1.f;
        float   groundY = 0.f;
        bool    destroyOnGroundHit = true;
        bool    stopOnGroundHit = false;
    };

public:
    explicit MeshDebrisObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit MeshDebrisObject(const MeshDebrisObject& rhs);
    virtual ~MeshDebrisObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

private:
    HRESULT Ready_Components(const FMeshDebrisDesc& desc);
    void Resolve_GroundHit();

private:
    // 실제 비주얼 레이어를 재생할 effect component다.
    Shared<EffectComponent> _effectCom;

private:
    string _effectAssetName;

    Vec3 _velocity = Vec3::Zero;

    float _gravity = -20.f;
    Vec3 _angularVelocityDeg = Vec3::Zero;

    float _lifetime = 1.f;
    float _elapsedTime = 0.f;
    float _groundY = 0.f;

    bool _destroyOnGroundHit = true;
    bool _stopOnGroundHit = false;
    bool _hasHitGround = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
