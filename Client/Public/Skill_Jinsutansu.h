#pragma once

#include "SkillObject.h"

NS_BEGIN(Client)

class Skill_Jinsutansu : public SkillObject
{
    GENERATED_BODY(Skill_Jinsutansu)

public:
    explicit Skill_Jinsutansu(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit Skill_Jinsutansu(const Skill_Jinsutansu& rhs);
    virtual ~Skill_Jinsutansu() = default;
    
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
