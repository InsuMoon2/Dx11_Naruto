#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Client)

class Skill_RasenShuriken : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_RasenShuriken)

public:
    explicit Skill_RasenShuriken(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_RasenShuriken(const Skill_RasenShuriken& rhs);
    virtual ~Skill_RasenShuriken() = default;
    
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

    void        Trigger_FinalHit(Character* character, GameObject* targetKey);

private:
    bool        _isActivatedByHit = false;
    Vec3        _activationPosition = Vec3::Zero;

    float       _baseScale = 1.0f;
    float       _maxScale = 2.4f;
    float       _scaleGrowSpeed = 2.8f;

    float       _midHitLaunchUp = 4.5f;

    // 터질 때
    float       _finalBlastRadius = 4.5f;
    float       _finalBlastLaunchPower = 140.f;
    float       _finalBlastLaunchUp = 3.5f;

    // 터졌는지?
    bool _isFinalBlast = false;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
