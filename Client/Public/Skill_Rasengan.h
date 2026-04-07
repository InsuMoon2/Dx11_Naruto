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
