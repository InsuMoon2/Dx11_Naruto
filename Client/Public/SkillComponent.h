#pragma once

#include "Component.h"

NS_BEGIN(Client)

class CombatStat;
class SkillObject_Projectile;

enum class ESkillType
{
    Rasengan        = 1001,
    Rasen_Shuriken  = 1002,
    Fireball,
    Chidori,
    Big_Rasengan,

    END
};

class SkillComponent : public Component
{
    GENERATED_COMPONENT(SkillComponent, Protocol::COMPONENT_TYPE_SKILL)

public:
    struct FSkillDesc
    {
        int slotSkill_Id[2] = { 0, 0 };
    };

public:
    explicit SkillComponent(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit SkillComponent(const SkillComponent& rhs);
    virtual ~SkillComponent() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Update(float timeDelta);

public:
    bool    Try_Activate(int slot);
    int     Get_EquippedSkillID(int slot) const;
    float   Get_CooldownRatio(int slot) const;

public:
    void    Set_PendingSkill(Protocol::OBJECT_TYPE type, Shared<SkillObject_Projectile> skill);
    bool    Launch_PendingSkill(Protocol::OBJECT_TYPE type, const Vec3& direction);
    void    Clear_PendingSkill(Protocol::OBJECT_TYPE type);

    // On_Tick에서 위치 동기화할 때 사용
    Weak<SkillObject_Projectile> Get_PendingSkill(Protocol::OBJECT_TYPE type) const;

protected:
    json    To_Json() const override;
    void    From_Json(const json& data) override;

private:
    static constexpr int32 SLOT_COUNT = 2; // 스킬은 일단 2개만

    int32   _slotSkill_Id[SLOT_COUNT] = {};
    float   _cooldownRemain[SLOT_COUNT] = {};

    Weak<CombatStat> _combatStat;

    umap<Protocol::OBJECT_TYPE, Weak<SkillObject_Projectile>> _pendingSkills;

public:
    static Shared<SkillComponent> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<Component> Clone(void* arg) override;
    void Free() override;
};

NS_END
