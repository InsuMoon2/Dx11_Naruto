#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Rasengan : public SkillObject
{
    GENERATED_BODY(Skill_Rasengan)

public:
    explicit Skill_Rasengan(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Rasengan(const Skill_Rasengan& rhs);
    virtual ~Skill_Rasengan() = default;
    
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
