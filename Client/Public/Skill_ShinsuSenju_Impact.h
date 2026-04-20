#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Collider;
class Character;
NS_END

NS_BEGIN(Client)

class Skill_ShinsuSenju_Impact final : public SkillObject
{
    GENERATED_BODY(Skill_ShinsuSenju_Impact)

public:
    struct FImpactDesc : public FSkillObjectDesc
    {
        float damage = 30.f;
        float launchForce = 5.5f;
        float launchUp = 2.0f;
        float lingerTime = 0.12f;
        Vec3 effectScale = Vec3(1.f, 1.f, 1.f);
    };

public:
    explicit Skill_ShinsuSenju_Impact(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_ShinsuSenju_Impact(const Skill_ShinsuSenju_Impact& rhs);
    virtual ~Skill_ShinsuSenju_Impact() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

public:
    void OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    Character* Find_HitCharacter(Shared<Collider> other);
    void Process_Hit(Character* hitted, GameObject* targetKey);
    HRESULT Play_ImpactEffect(const FImpactDesc& desc);

    void Spawn_Particle(const string& assetName, int32 spawnIndex, const Vec3 spawnPos);

private:
    float _damage = 30.f;
    float _launchUp = 2.0f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
