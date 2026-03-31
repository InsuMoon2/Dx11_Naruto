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

private:
    float _speed = 28.f;
    float _maxDistance = 35.f;
    float _lifetime = 2.5f;

    float _colliderRadius = 1.35f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
