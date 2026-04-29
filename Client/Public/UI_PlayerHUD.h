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

enum class ECineTransitionState
{
    Idle,           
    FadeOutToBlack, // 진입 시 까매짐
    FadeInToCine,   // 시네마틱 시작하며 밝아짐
    Playing,        // 시네마틱 재생 중 대기
    FadeInToGame    // 시네마틱 종료 후 겜 화면으로 밝아짐
};

class UI_PlayerHUD : public HUD
{
    GENERATED_BODY(UI_PlayerHUD)

private:
    struct FRemotePlayerStatusEntry
    {
        uint64 networkId = 0; 
        Weak<Player> player; 
        Weak<CombatStat> combat;
        Shared<Background> hpBackground;
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

    void        Show_MissionTitle(const wstring& text, float displayTime = 2.f);
    void        Hide_MissionTitle();

public:
    void        Show_MissionEnd();
    void        Hide_MissionEnd();
    void        Set_MissionEndOpacity(float alpha);
    void        Set_ScreenFadeAlpha(float alpha);

public: /* 시네마틱 공통 */
    void        Handle_WaveStarted(const string& waveTag);
    HRESULT     Ready_CinematicTransition();
    void        Update_CinematicTransition(float timeDelta);
    void        Set_HUDVisibilityForCinematic(bool isVisible);
    void        On_CinematicFinished();

    /** 코노하 레벨 입장 시 자동 재생되는 레벨 진입 시네마틱을 트리거한다.
     *  Ready_UI() 완료 후 Level_Konoha::Ready_UI()에서 직접 호출한다. */
    void        Trigger_LevelEntryCinematic(
        const string& cinematicTag,
        const wstring& introSoundFile = L"",
        const wstring& gameplayBgmFile = L"");

    FDelegateHandle             _waveStartedHandle = {};
    FDelegateHandle             _cineFinishedHandle = {};

private:

    ECineTransitionState        _cineTransitionState = ECineTransitionState::Idle; 
    float                       _cineTransitionTimer = 0.f;                        
    
    string                      _pendingCinematicTag = "";                         
    wstring                     _pendingLevelEntryIntroSoundFile = L""; // 레벨 입장 컷신이 시작될 때 1회 재생할 전용 사운드 파일 경로다.
    wstring                     _pendingLevelEntryGameplayBgmFile = L""; // 레벨 입장 컷신이 끝나고 Mission Start가 뜰 때 재생할 메인 BGM 파일 경로다.
    bool                        _isCineDialoguePlayed = false;
    bool                        _isLevelEntryCinematic = false; // 레벨 진입 시네마틱 여부 (보스와 구분)
    
    static constexpr float      CINE_FADE_OUT_TIME = 0.4f;   

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
    HRESULT     Ready_MissionTitleUI();
    void        Update_MissionTitle(float timeDelta);

private:
    HRESULT     Ready_UI(void* arg);
    HRESULT     Ready_Timer();
    HRESULT     Ready_WireLockOn();
    HRESULT     Ready_MissionClearUI();

    /** 코노하 시네마틱 종료 후 전투 개시 텍스트를 표시하는 산율 연출을 시작한다. */
    void        Show_BattleStartAnnounce();
    /** 매 프레임 코 업 + 페이드 아웃 연출을 업데이트한다. */
    void        Update_BattleStartAnnounce(float timeDelta);


public:
    FOnHUDPlayerBound OnHUDPlayerBound;
    FDelegateHandle   _wireLockOnVisibleHandle = {};

    FDelegateHandle _missionMarkerTargetHandle = {};
    FDelegateHandle _missionMarkerClearHandle = {};

private:
    Weak<GameObject>        _pendingBossObject;
    Shared<UI_PlayerStatus>     _status;
    Shared<UI_PlayerSkill>      _skillPanel;
    Shared<UI_AnnounceCombo>    _announceCombo;
    Shared<UI_Targeting>        _targeting;
    Shared<UI_Timer>            _timer;
    Shared<UI_WireLockOn>       _wireLockOn;
    Shared<UI_MissionMarker>    _missionMarker;
    Shared<Background>          _missionTitleBanner;

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
    Shared<Background>          _bossCineText;
    Shared<Background>          _bossLastFinish;

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

    Shared<UI_ScreenFade>   _letterBoxTop;
    Shared<UI_ScreenFade>   _letterBoxBottom;

    Shared<Background>      _battleStartBanner; // 전투 개시 고지 (TEXTURE_MISSION 인덱스 1)

    float               _missionTitleTimer = 0.f;
    float               _missionTitleBaseX = 0.f;

private:
    static constexpr float MISSION_TITLE_FADE_TIME   = 0.35f;
    static constexpr float MISSION_TITLE_SLIDE_DIST  = 80.f;

    // 전투 개시 연출 상수
    static constexpr float  BATTLE_START_HOLD_TIME  = 0.5f;  // 케이지 전 홀드 시간
    static constexpr float  BATTLE_START_ZOOM_TIME  = 0.6f;  // 케이지의 스케일업 + 페이드 아웃 시간
    static constexpr float  BATTLE_START_SCALE_END  = 1.5f;  // 케이지 완료 시 스케일
    float                   _battleStartTimer = -1.f;         // 음수 = 비활성

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
