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
class Background;

class CombatStat;
class UI_PlayerHP;

DECLARE_DELEGATE(FOnHUDPlayerBound, Shared<Player>);

class UI_PlayerHUD : public HUD
{
    GENERATED_BODY(UI_PlayerHUD)

private:
    struct FRemotePlayerStatusEntry
    {
        uint64 networkId = 0; 
        Weak<Player> player; 
        Weak<CombatStat> combat;
        Shared<UI_PlayerHP> hpBar;
        Shared<UI_Text> nameText; 
    };

    struct FCombatLineLayer
    {
        Shared<Background> widget;
        uint32 textureIndex = 0;
        float basePosX = 0.f;
        float basePosY = 0.f;
        float baseSizeX = 0.f;
        float baseSizeY = 0.f;
        float baseRotation = 0.f;
        float alphaWeight = 1.f;
        float scaleJitter = 0.f;
        float phaseOffset = 0.f;
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
    void        Add_RemotePlayer(Shared<Player> player);
    void        Update_RemotePlayerStatusList();
    bool        Has_RemotePlayerStatus(uint64 networkId) const;

    void        Handle_RemotePlayerObjectSpawned(Shared<GameObject> obj);
    void        On_PlayerComboHit(uint32 combo);

private:
    HRESULT     Ready_CombatLines();
    void        Reset_CombatLineLayout();
    void        Update_CombatLineBurst(float timeDelta);
    float       Compute_CombatLineAlpha() const;
    float       Compute_CombatLineIntensity(uint32 combo) const;
    void        Trigger_CombatLineBurst(float intensity);
    void        Stop_CombatLineBurst();

private:
    HRESULT     Ready_UI(void* arg);

public:
    FOnHUDPlayerBound OnHUDPlayerBound;
    

private:
    Shared<UI_PlayerStatus>     _status;
    Shared<UI_PlayerSkill>      _skillPanel;
    Shared<UI_AnnounceCombo>    _announceCombo;
    Shared<UI_Targeting>        _targeting;

    vector<FCombatLineLayer>    _combatLineLayers;
    FDelegateHandle             _remotePlayerSpawnedHandle = {};
    FDelegateHandle             _comboHitHandle = {};
    vector<FRemotePlayerStatusEntry> _remotePlayerStatuses;
    float                       _combatLineHoldTimer = 0.f;
    float                       _combatLineFadeTimer = 0.f;
    float                       _combatLinePhaseTime = 0.f;
    float                       _combatLineIntensity = 1.f;
    bool                        _isCombatLineBurstActive = false;

private:
    static constexpr uint32 COMBAT_LINE_TEXTURE_INDEX_07 = 8;
    static constexpr float COMBAT_LINE_HOLD_TIME = 0.06f;
    static constexpr float COMBAT_LINE_FADE_TIME = 0.18f;

public:
    static Shared<UI_PlayerHUD> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
