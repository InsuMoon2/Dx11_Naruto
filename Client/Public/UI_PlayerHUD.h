#pragma once

#include "HUD.h"

NS_BEGIN(Client)

class Player;
class UI_PlayerStatus;
class UI_PlayerSkill;
class UI_AnnounceCombo;

DECLARE_DELEGATE(FOnHUDPlayerBound, Shared<Player>);

class UI_PlayerHUD : public HUD
{
    GENERATED_BODY(UI_PlayerHUD)

public:
    explicit UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerHUD(const UI_PlayerHUD& rhs);
    virtual ~UI_PlayerHUD() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Update(float timeDelta) override;

    void        Bind_Player(Shared<Player> player);

private:
    HRESULT     Ready_UI(void* arg);

public:
    FOnHUDPlayerBound OnHUDPlayerBound;

private:
    Shared<UI_PlayerStatus> _status;
    Shared<UI_PlayerSkill>  _skillPanel;
    Shared<UI_AnnounceCombo> _announceCombo;

public:
    static Shared<UI_PlayerHUD> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
