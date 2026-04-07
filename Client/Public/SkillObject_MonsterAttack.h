#pragma once

#include "SkillObject.h"

NS_BEGIN(Engine)
class Collider;
class Character;
NS_END

NS_BEGIN(Client)

class SkillObject_MonsterAttack final : public SkillObject
{
    GENERATED_BODY(SkillObject_MonsterAttack)

public:
    explicit SkillObject_MonsterAttack(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillObject_MonsterAttack(const SkillObject_MonsterAttack& rhs);
    virtual ~SkillObject_MonsterAttack() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg)  override;

public:
    void OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void OnStayOverlap(Shared<Collider> self, Shared<Collider> other) override;
    void OnEndOverlap(Shared<Collider> self, Shared<Collider> other) override;

private:
    Character* Find_HitCharacter(Shared<Collider> other);
    void Process_Hit(Character* hitted, GameObject* targetKey);

private:
    float _damage = 15.f;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
