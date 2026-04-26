#pragma once

#include "HUD.h"

NS_BEGIN(Engine)
class UI_Text;
class Character;
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
class UI_BossHp;
class Monster;
class UI_Timer;
class UI_WireLockOn;
class UI_MissionMarker;
class UI_ScreenFade;

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

    void        Handle_BossSpawned(Shared<GameObject> obj);

    void        Handle_CharacterDead(Shared<Character> deadCharacter, Shared<GameObject> damageCauser);
    void        Handle_WireLockOnVisible(bool visible);

    void        Show_WireLockOn();
    void        Hide_WireLockOn();

    void        Set_MissionMarkerTarget(Shared<GameObject> targetObject);
    void        Clear_MissionMarkerTarget();

public:
    void        Show_MissionEnd();
    void        Hide_MissionEnd();
    void        Set_MissionEndOpacity(float alpha);
    void        Set_ScreenFadeAlpha(float alpha);

private:
    HRESULT     Ready_CombatLines();
    void        Reset_CombatLineLayout();
    void        Update_CombatLineBurst(float timeDelta);
    float       Compute_CombatLineAlpha() const;
    float       Compute_CombatLineIntensity(uint32 combo) const;
    void        Trigger_CombatLineBurst(float intensity);
    void        Stop_CombatLineBurst();

    HRESULT     Ready_KOAnnounce();
    void        Show_KOAnnounce();
    void        Hide_KOAnnounce();
    void        Update_KOAnnounce(float timeDelta);
    void        Apply_AnnouncePositions();

private:
    HRESULT     Ready_UI(void* arg);
    HRESULT     Ready_Timer();
    HRESULT     Ready_WireLockOn();
    HRESULT     Ready_MissionClearUI();


public:
    FOnHUDPlayerBound OnHUDPlayerBound;
    FDelegateHandle   _wireLockOnVisibleHandle = {};

    FDelegateHandle _missionMarkerTargetHandle = {};
    FDelegateHandle _missionMarkerClearHandle = {};

private:
    Shared<UI_PlayerStatus>     _status;
    Shared<UI_PlayerSkill>      _skillPanel;
    Shared<UI_AnnounceCombo>    _announceCombo;
    Shared<UI_Targeting>        _targeting;
    Shared<UI_Timer>            _timer;
    Shared<UI_WireLockOn>       _wireLockOn;
    Shared<UI_MissionMarker>    _missionMarker;

    vector<FCombatLineLayer>    _combatLineLayers;
    FDelegateHandle             _remotePlayerSpawnedHandle = {};
    FDelegateHandle             _comboHitHandle = {};
    vector<FRemotePlayerStatusEntry> _remotePlayerStatuses;
    float                       _combatLineHoldTimer = 0.f;
    float                       _combatLineFadeTimer = 0.f;
    float                       _combatLinePhaseTime = 0.f;
    float                       _combatLineIntensity = 1.f;
    bool                        _isCombatLineBurstActive = false;

    Shared<Background>          _bossGauge;
    Shared<Background>          _bossIcon;

    Shared<UI_BossHp>           _bossHp;
    FDelegateHandle             _bossObjectSpawnedHandle = {};

    Shared<Background>          _koBackground;       
    Shared<Background>          _koText;             
    FDelegateHandle             _deadHandle = {};    
    float                       _koVisibleTimer = 0.f; 
    bool                        _isKOVisible = false;  

    float                       _comboAnnounceOffsetX = 35.f;
    float                       _comboAnnounceOffsetY = 35.f;
    float                       _koAnnounceOffsetX = 167.f;
    float                       _koAnnounceOffsetY = -14.f;

    Shared<Background>      _missionEndBanner;
    Shared<UI_ScreenFade>   _screenFadePanel;

private:
    static constexpr uint32 COMBAT_LINE_TEXTURE_INDEX_07 = 8;
    static constexpr float  COMBAT_LINE_HOLD_TIME = 0.06f;
    static constexpr float  COMBAT_LINE_FADE_TIME = 0.18f;

    static constexpr float  KO_ANNOUNCE_VISIBLE_TIME = 1.2f; 
    static constexpr float  KO_ANNOUNCE_INTRO_TIME = 0.12f;  
    static constexpr float  KO_ANNOUNCE_FADE_TIME = 0.25f;   
    static constexpr float  KO_BG_FADE_IN_TIME = 0.2f;
    static constexpr float  KO_TEXT_DELAY_TIME = 0.2f;
    static constexpr float  KO_ANNOUNCE_ROTATION_DEGREE = 30.f;
    static constexpr float  KO_ANNOUNCE_START_SCALE = 1.12f; 
    static constexpr float  KO_BG_BASE_WIDTH = 304.f;    
    static constexpr float  KO_BG_BASE_HEIGHT = 240.f;   
    static constexpr float  KO_TEXT_BASE_WIDTH = 512.f;  
    static constexpr float  KO_TEXT_BASE_HEIGHT = 128.f;  

public:
    static Shared<UI_PlayerHUD> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    virtual void Free() override;

};

NS_END
