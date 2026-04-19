#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Kirin_Hit : public SkillObject
{
    GENERATED_BODY(Skill_Kirin_Hit)

public:
    explicit Skill_Kirin_Hit(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Kirin_Hit(const Skill_Kirin_Hit& rhs);
    virtual ~Skill_Kirin_Hit() = default;
    
public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    Update(float timeDelta) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void    OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    class Character* Find_HitCharacter(Shared<Collider> other);
    void             Process_MultiHit(class Character* hitted, GameObject* targetKey);



public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;

};

NS_END
