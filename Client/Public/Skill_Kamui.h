#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Kamui final : public SkillObject
{
    GENERATED_BODY(Skill_Kamui)

public:
    explicit Skill_Kamui(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Kamui(const Skill_Kamui& rhs);
    virtual ~Skill_Kamui() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    Character*  Find_HitCharacter(Shared<Collider> other);
    void        Process_MultiHit(Character* hitted, GameObject* targetKey);

private:
    float _baseDamage = 6.f; 
    float _finalDamage = 14.f; 

    float _midHitLaunchForce = 0.f;
    float _midHitLaunchUp = 0.f; 

    float _finalHitLaunchForce = 8.f;
    float _finalHitLaunchUp = 2.f;

    int32 _currentHitStep = 0; 
    umap<GameObject*, int32> _lastAppliedStepByTarget;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
