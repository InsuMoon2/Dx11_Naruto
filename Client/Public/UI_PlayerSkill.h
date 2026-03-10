#pragma once

#include "Panel.h"

NS_BEGIN(Client)

class Player;
class UI_SkillSlot;

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

public:
    void Bind_Player(Shared<Player> player) { _player = player; }

private:
    Weak<Player>         _player;
    Shared<UI_SkillSlot> _slots[2];

public:
    static Shared<UI_PlayerSkill> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg = nullptr);
    void Free() override;
};

NS_END
