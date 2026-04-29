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
        float   lifetime      = 0.f;

        Vec3    direction = Vec3::Forward;
        Vec3    attachOffset = Vec3::Zero;

        int32   ownerSkillId  = 0;

        string  effectAssetName = "";
        Collision_Preset collisionPreset = Collision_Preset::Projectile;
        Shared<GameObject> ownerObject = nullptr;
        bool useHitReactionOverride = false; // SpawnAttack/스킬이 맞힌 대상의 히트 리액션 타입을 강제로 지정할지 여부다.
        EHitReactionType hitReactionType = EHitReactionType::Default; // useHitReactionOverride가 켜졌을 때 FDamageEvent에 실어 보낼 히트 리액션 타입이다.
        bool useLaunchOverride = false; // SpawnAttack/스킬이 기본 launch 대신 지정한 launch 값을 강제로 사용할지 여부다.
        float launchPower = 0.f; // useLaunchOverride가 켜졌을 때 FDamageEvent에 실어 보낼 수평 launch 세기다.
        float launchUp = 0.f; // useLaunchOverride가 켜졌을 때 FDamageEvent에 실어 보낼 수직 launch 세기다.
        bool useHitSoundOverride = false; // SpawnAttack/스킬이 콤보 프로파일 대신 지정한 피격음을 강제로 사용할지 여부다.
        int32 hitSound = 0; // useHitSoundOverride가 켜졌을 때 FDamageEvent에 실어 보낼 정수 기반 피격 사운드 ID다.
        string hitSoundFile = ""; // useHitSoundOverride가 켜졌을 때 FDamageEvent에 실어 보낼 직접 피격 사운드 파일명이다.

        bool useDirectionLookAt = true;
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
    bool    Should_ExcludeFromEditorSnapshot() const override { return true; }

    virtual void Sync_AttachedTransform(const Matrix& boneWorldMatrix);

    static Shared<GameObject> Spawn_Effect_Once(
        const string& effectAssetName, const Vec3& worldPosition, const Vec3 worldScale = Vec3::One);

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

    Vec3    _attachOffset = Vec3::Zero;

protected:
    // 다단히트
    int32   _hitCount = 0;
    int32   _maxHitCount = 1;
    float   _hitInterval = 0.1f;
    float   _hitLaunchForce = 0.f;

    float   _colliderRadius = 1.0f;

    string  _effectAssetName = "";
    bool    _useHitReactionOverride = false; // 이 스폰 공격이 기본 리액션 대신 지정한 히트 리액션 타입을 강제할지 여부다.
    EHitReactionType _hitReactionType = EHitReactionType::Default; // _useHitReactionOverride가 true일 때 Apply_Skill_Hit에서 사용할 히트 리액션 타입이다.
    bool    _useLaunchOverride = false; // 이 스폰 공격이 Apply_Skill_Hit에 전달된 기본 launch 대신 지정 launch 값을 강제할지 여부다.
    float   _launchPower = 0.f; // _useLaunchOverride가 true일 때 Apply_Skill_Hit에서 사용할 수평 launch 세기다.
    float   _launchUp = 0.f; // _useLaunchOverride가 true일 때 Apply_Skill_Hit에서 사용할 수직 launch 세기다.
    bool    _useHitSoundOverride = false; // 이 스폰 공격이 기본 피격음 대신 지정한 피격음을 강제할지 여부다.
    int32   _hitSound = 0; // _useHitSoundOverride가 true일 때 Apply_Skill_Hit에서 사용할 정수 기반 피격 사운드 ID다.
    string  _hitSoundFile = ""; // _useHitSoundOverride가 true일 때 Apply_Skill_Hit에서 사용할 직접 피격 사운드 파일명이다.

    // 동일 대상 충돌 처리
    umap<GameObject*, float> _hitCooldowns;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
    
};

NS_END
