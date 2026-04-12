#pragma once

#include "Panel.h"

NS_BEGIN(Client)

class Player;
class UI_SkillSlot;
class CombatStat;
class UI_WeaponType;

class UI_PlayerSkill final : public Panel
{
    GENERATED_BODY(UI_PlayerSkill)

public:
    explicit UI_PlayerSkill(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerSkill(const UI_PlayerSkill& rhs);
    virtual ~UI_PlayerSkill() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;
    void    BeginPlay() override;
    void    Update(float timeDelta) override;

    void    On_WeaponTypeChanged(int32 weaponTypeIndex);

public:
    void    Bind_Player(Shared<Player> player);

private:
    HRESULT Ready_Skill(void* arg);

private:
    Weak<Player>         _player;
    Weak<CombatStat>     _combat;

    Shared<UI_SkillSlot> _skillSlots[2];
    Shared<UI_SkillSlot> _subSkillSlot[2];

    Shared<UI_WeaponType> _weaponTypeUI;

    FDelegateHandle       _weaponTypeHandle;

public:
    static Shared<UI_PlayerSkill> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
