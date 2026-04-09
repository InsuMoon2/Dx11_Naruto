#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class Collider;
class Character;
class EffectComponent;
NS_END

NS_BEGIN(Client)

class SkillObject : public GameObject
{
    GENERATED_BODY(SkillObject)

public:
    struct FSkillObjectDesc : public FGameObjectDesc
    {
        Protocol::ComponentID colliderType = Protocol::COMPONENT_TYPE_COLLIDER_SPHERE;

        float   colliderRadius = 1.f;                       
        Vec3    colliderExtents = Vec3(0.5f, 0.5f, 0.5f);

        Vec3    spawnPosition = Vec3::Zero; 
        Vec3    spawnRotation = Vec3::Zero; 
        Vec3    scale         = Vec3::One;  
        float   lifetime      = 2.f;

        Vec3    direction = Vec3::Forward;

        int32   ownerSkillId  = 0;

        string  effectAssetName = "";
        Collision_Preset collisionPreset = Collision_Preset::Projectile;
    };

public:
    explicit SkillObject(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject(const SkillObject& rhs);
    virtual ~SkillObject() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Priority_Update(float timeDelta) override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;

    virtual void Sync_AttachedTransform(const Matrix& boneWorldMatrix);

protected:
    HRESULT Ready_Components(const FSkillObjectDesc& desc);

    bool    Apply_Skill_Hit(Character* hitted, float damage, float launchForce = 0.f, float launchUp = 0.f);

protected:
    Shared<Collider> _collider;
    Shared<EffectComponent> _effectCom;

    float   _lifetime = 2.f;        // 수명 시간     
    float   _elapsedTime = 0.f;     // 스폰 후 경과 시간

    int32   _ownerSkillId = 0;

    Collision_Preset   _collisionPreset = Collision_Preset::Projectile;

protected:
    // 다단히트
    int32   _hitCount = 0;
    int32   _maxHitCount = 1;
    float   _hitInterval = 0.1f;
    float   _hitLaunchForce = 0.f;

    float   _colliderRadius = 1.0f;

    string  _effectAssetName = "";

    // 동일 대상 충돌 처리
    umap<GameObject*, float> _hitCooldowns;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
