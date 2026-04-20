#pragma once

#include "HUD.h"

NS_BEGIN(Engine)
class UI_Text;
NS_END

NS_BEGIN(Client)

class Player;
class UI_PlayerStatus;
class UI_PlayerSkill;
class UI_AnnounceCombo;
class UI_Targeting;

class CombatStat;
class UI_PlayerHP;

DECLARE_DELEGATE(FOnHUDPlayerBound, Shared<Player>);

class UI_PlayerHUD : public HUD
{
    GENERATED_BODY(UI_PlayerHUD)

private:
    // 원격 플레이어 값도 동기화 되도록
    struct FRemotePlayerStatusEntry
    {
        uint64 networkId = 0; 
        Weak<Player> player; 
        Weak<CombatStat> combat;
        Shared<UI_PlayerHP> hpBar;
        Shared<UI_Text> nameText; 
    };

public:
    explicit UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit UI_PlayerHUD(const UI_PlayerHUD& rhs);
    virtual ~UI_PlayerHUD() = default;

public:
    HRESULT     Initialize_Prototype() override;
    HRESULT     Initialize(void* arg) override;
    void        Update(float timeDelta) override;

    void        Bind_Player(Shared<Player> player);

public:
    void Add_RemotePlayer(Shared<Player> player);
    void Update_RemotePlayerStatusList();
    bool Has_RemotePlayerStatus(uint64 networkId) const;

    void Handle_RemotePlayerObjectSpawned(Shared<GameObject> obj);

private:
    HRESULT     Ready_UI(void* arg);

public:
    FOnHUDPlayerBound OnHUDPlayerBound;
    

private:
    Shared<UI_PlayerStatus>     _status;
    Shared<UI_PlayerSkill>      _skillPanel;
    Shared<UI_AnnounceCombo>    _announceCombo;
    Shared<UI_Targeting>        _targeting;

    // 음.. 근데 그냥 한명만 접속할거긴 함
    vector<FRemotePlayerStatusEntry> _remotePlayerStatuses;

public:
    static Shared<UI_PlayerHUD> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
