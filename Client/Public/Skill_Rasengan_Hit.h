#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Rasengan_Hit : public SkillObject
{
    GENERATED_BODY(Skill_Rasengan_Hit)

public:
    struct FHitDesc : public FSkillObjectDesc
    {
    };

public:
    explicit Skill_Rasengan_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Rasengan_Hit(const Skill_Rasengan_Hit& rhs);
    virtual ~Skill_Rasengan_Hit() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    // 충둘된 놈이 실제로 타격 가능한지 판단
    Character*  Find_HitCharacter(Shared<Collider> other);

    void        Process_MultiHit(Character* hitted, GameObject* targetKey);

private:
    float _baseDamage = 10.f;
    float _finalDamage = 20.f;

    float _midHitLaunchUp = 0.0f;
    float _finalHitLaunchForce = 15.f;

    float _finalHitLaunchUp = 4.f;

    int32 _currentHitStep = 0;
    umap<GameObject*, int32> _lastAppliedStepByTarget;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
