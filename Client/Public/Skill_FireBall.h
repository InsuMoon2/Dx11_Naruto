#pragma once

#include "SkillObject_Projectile.h"

NS_BEGIN(Client)

class Skill_FireBall : public SkillObject_Projectile
{
    GENERATED_BODY(Skill_FireBall)

public:
    explicit Skill_FireBall(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_FireBall(const Skill_FireBall& rhs);
    virtual ~Skill_FireBall() = default;
    
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

    void        Process_Hit(Character* hitted, GameObject* targetKey);

private:
    float       _launchPower = 140.f;
    float       _launchUp = 3.5f;


public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
