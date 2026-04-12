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

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};


NS_END
