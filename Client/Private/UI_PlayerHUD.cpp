#include "pch.h"
#include "UI_PlayerHUD.h"
#include "GameInstance.h"
#include "UI_PlayerStatus.h"
#include "UI_PlayerSkill.h"
#include "GameObject_Factory.h"
#include "UI_AnnounceCombo.h"
#include "Player.h"
#include "TargetComponent.h"
#include "UI_Targeting.h"
#include "UI_PlayerHP.h"
#include "UI_Text.h"
#include "CombatStat.h"
#include "Background.h"
#include "Level_CharacterSetup.h"
#include "GameInstance.h"
#include "UI_BossHp.h"
#include "Monster.h"
#include "Character.h"
#include "UI_MissionMarker.h"
#include "UI_ScreenFade.h"
#include "UI_Timer.h"
#include "UI_WireLockOn.h"

REGISTER_GAMEOBJECT(UI_PlayerHUD, Protocol::OBJECT_TYPE_UI_PLAYER_HUD)

IMPLEMENT_REFLECTION(UI_PlayerHUD)

bool UI_PlayerHUD::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "UI_PlayerHUD";

    PROPERTY_UIOBJECT_FORCE_VISIBLE();

    PROPERTY_FLOAT("Combo Announce Offset X", _comboAnnounceOffsetX, -1000.f, 1000.f);
    PROPERTY_FLOAT("Combo Announce Offset Y", _comboAnnounceOffsetY, -1000.f, 1000.f);
    PROPERTY_FLOAT("KO Announce Offset X", _koAnnounceOffsetX, -1000.f, 1000.f);
    PROPERTY_FLOAT("KO Announce Offset Y", _koAnnounceOffsetY, -1000.f, 1000.f);

    return true;
}

UI_PlayerHUD::UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : HUD(device, context)
{

}

UI_PlayerHUD::UI_PlayerHUD(const UI_PlayerHUD& rhs)
    : HUD(rhs)
    , _comboAnnounceOffsetX(rhs._comboAnnounceOffsetX)
    , _comboAnnounceOffsetY(rhs._comboAnnounceOffsetY)
    , _koAnnounceOffsetX(rhs._koAnnounceOffsetX)
    , _koAnnounceOffsetY(rhs._koAnnounceOffsetY)
{
}

HRESULT UI_PlayerHUD::Initialize_Prototype()
{
    return HUD::Initialize_Prototype();
}

HRESULT UI_PlayerHUD::Initialize(void* arg)
{
    CHECK_FAILED(HUD::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_UI(arg), E_FAIL);

    OnHUDPlayerBound.Add(_status.get(), &UI_PlayerStatus::Bind_Player);
    OnHUDPlayerBound.Add(_skillPanel.get(), &UI_PlayerSkill::Bind_Player);

    _remotePlayerSpawnedHandle = GAME->Get_DelegateHub().OnRemotePlayerObjectSpawned.Add(
        this, &UI_PlayerHUD::Handle_RemotePlayerObjectSpawned);

    _comboHitHandle = GAME->Get_DelegateHub().OnPlayerComboHit.Add(
        this, &UI_PlayerHUD::On_PlayerComboHit);

    _bossObjectSpawnedHandle = GAME->Get_DelegateHub().OnBossObjectSpawned.Add(
        this, &UI_PlayerHUD::Handle_BossSpawned);

    _deadHandle = GAME->Get_DelegateHub().OnDead.Add(
        this, &UI_PlayerHUD::Handle_CharacterDead);

    _wireLockOnVisibleHandle = GAME->Get_DelegateHub().OnWireLockOnVisible.Add(
        this, &UI_PlayerHUD::Handle_WireLockOnVisible);

    _missionMarkerTargetHandle = GAME->Get_DelegateHub().OnMissionMarkerTargetChanged.Add(
        this, &UI_PlayerHUD::Set_MissionMarkerTarget);

    _missionMarkerClearHandle = GAME->Get_DelegateHub().OnMissionMarkerTargetCleared.Add(
        this, &UI_PlayerHUD::Clear_MissionMarkerTarget);

    if (auto missionMarkerTarget = GAME->Get_DelegateHub().Get_MissionMarkerTarget())
        Set_MissionMarkerTarget(missionMarkerTarget);

    _waveStartedHandle = GAME->Get_DelegateHub().OnWaveStarted.Add(
        this, &UI_PlayerHUD::Handle_WaveStarted);

    _cineFinishedHandle = GAME->Get_DelegateHub().OnCinematicFinished.Add(
        this, &UI_PlayerHUD::On_CinematicFinished);

    return S_OK;
}

void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);

    Update_RemotePlayerStatusList();
    Update_CombatLineBurst(timeDelta);
    Update_KOAnnounce(timeDelta);
    Update_MissionTitle(timeDelta);
    Update_CinematicTransition(timeDelta);
    Update_BattleStartAnnounce(timeDelta);

    Apply_AnnouncePositions();

    const bool bossVisible = _bossHp && _bossHp->Is_Visibility();

    if (_bossGauge)
        _bossGauge->Set_Visibility(bossVisible);

    if (_bossIcon)
        _bossIcon->Set_Visibility(bossVisible);
}

void UI_PlayerHUD::Bind_Player(Shared<Player> player)
{
    OnHUDPlayerBound.Broadcast(player);

    CHECK_NULL(player);

    auto targetCom = player->Get_Component<TargetComponent>();
    CHECK_NULL(targetCom);

    if (_targeting)
        _targeting->Set_TargetComponent(targetCom);

}

void UI_PlayerHUD::Add_RemotePlayer(Shared<Player> player)
{
    if (!player)
        return;

    const uint64 networkId = player->Get_NetworkId();
    if (networkId == 0 || Has_RemotePlayerStatus(networkId))
        return;

    FRemotePlayerStatusEntry entry{};
    entry.networkId = networkId;
    entry.player = player;
    entry.combat = player->Get_Component<CombatStat>();

    Background::FBackgroundDesc hpBackgroundDesc{};
    hpBackgroundDesc.posX = 0.f;
    hpBackgroundDesc.posY = 0.f;
    hpBackgroundDesc.sizeX = 320.f;
    hpBackgroundDesc.sizeY = 96.f;
    hpBackgroundDesc.zOrder = _zOrder + 0.035f;
    hpBackgroundDesc.levelIndex = _levelIndex;
    hpBackgroundDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_EQUIPMENT;
    hpBackgroundDesc.textureIndex = ETOI(ECharacterSetupTexture::RemoteHpBackground);
    hpBackgroundDesc.shaderPassIndex = 1;

    entry.hpBackground = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND,
        EUILayer::HUD,
        &hpBackgroundDesc);

    UI_PlayerHP::FPlayerHPDesc hpDesc{};
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.sizeX = 280.f;
    hpDesc.sizeY = 42.f;
    hpDesc.zOrder = _zOrder + 0.04f;
    hpDesc.levelIndex = _levelIndex;
    hpDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_PLAYER_STATUS;

    entry.hpBar = Create_Child<UI_PlayerHP>(
        Protocol::OBJECT_TYPE_UI_PLAYER_HP,
        EUILayer::HUD,
        &hpDesc);

    if (entry.hpBar)
    {
        entry.hpBar->Set_FillRange(98.f / 512.f, 413.f / 512.f);
        entry.hpBar->Get_Transform()->Set_LocalScale(280.f, 42.f, 1.f);
    }

    UI_Text::FUITextDesc nameDesc{};
    nameDesc.posX = 0.f;
    nameDesc.posY = 0.f;
    nameDesc.sizeX = 280.f;
    nameDesc.sizeY = 30.f;
    nameDesc.zOrder = _zOrder + 0.05f;
    nameDesc.levelIndex = _levelIndex;
    nameDesc.text = player->Get_PlayerName().empty() ? L"Player" : player->Get_PlayerName();
    nameDesc.style.fontSize = 20.f;
    nameDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    nameDesc.style.hAlign = ETextHAlign::Center;
    nameDesc.style.vAlign = ETextVAlign::Middle;

    entry.nameText = Create_Child<UI_Text>(
        Protocol::OBJECT_TYPE_UI_TEXT,
        EUILayer::HUD,
        &nameDesc);

    _remotePlayerStatuses.push_back(entry);
}

void UI_PlayerHUD::Update_RemotePlayerStatusList()
{
    _remotePlayerStatuses.erase(
        remove_if(_remotePlayerStatuses.begin(), _remotePlayerStatuses.end(),
            [](FRemotePlayerStatusEntry& entry)
            {
                auto player = entry.player.lock();
                const bool shouldRemove = !player || player->Is_Destroy();

                if (shouldRemove)
                {
                    if (entry.hpBackground)
                        entry.hpBackground->Set_Destroy(true);

                    if (entry.hpBar)
                        entry.hpBar->Set_Destroy(true);

                    if (entry.nameText)
                        entry.nameText->Set_Destroy(true);
                }

                return shouldRemove;
            }),
        _remotePlayerStatuses.end());

    const float baseX = 20.f;
    const float baseY = 258.f;
    const float hpWidth = 280.f;
    const float hpHeight = 42.f;
    const float nameHeight = 30.f;
    const float nameGap = 4.f;
    const float backgroundHeight = 96.f;
    const float gapY = 104.f;

    for (size_t i = 0; i < _remotePlayerStatuses.size(); ++i)
    {
        auto& entry = _remotePlayerStatuses[i];

        const float y = baseY + static_cast<float>(i) * gapY;
        const float centerX = baseX + hpWidth * 0.5f;
        const float nameY = y + nameHeight * 0.5f;
        const float hpY = y + nameHeight + nameGap + hpHeight * 0.5f;
        const float backgroundY = y + backgroundHeight * 0.5f;

        if (auto hpBackground = entry.hpBackground)
            hpBackground->Get_Transform()->Set_LocalPosition(centerX, backgroundY, _zOrder + 0.035f);

        if (auto hpBar = entry.hpBar)
        {
            hpBar->Get_Transform()->Set_LocalPosition(centerX, hpY, _zOrder + 0.04f);

            auto combat = entry.combat.lock();
            hpBar->Set_Ratio(combat ? combat->Get_HpRatio() : 1.f);
        }

        if (auto nameText = entry.nameText)
        {
            auto player = entry.player.lock();
            if (player)
                nameText->Set_Text(player->Get_PlayerName());

            nameText->Get_Transform()->Set_LocalPosition(centerX, nameY, _zOrder + 0.05f);
        }
    }
}

bool UI_PlayerHUD::Has_RemotePlayerStatus(uint64 networkId) const
{
    for (const auto& entry : _remotePlayerStatuses)
    {
        if (entry.networkId == networkId)
            return true;
    }

    return false;
}

void UI_PlayerHUD::Handle_RemotePlayerObjectSpawned(Shared<GameObject> obj)
{
    auto player = dynamic_pointer_cast<Player>(obj);
    if (!player)
        return;

    Add_RemotePlayer(player);
}

void UI_PlayerHUD::On_PlayerComboHit(uint32 combo)
{
    // 콤보 히트 시 화면 가장자리 Radial/CombatLine 연출을 잠시 비활성화한다.
    // 필요할 때 아래 버스트 트리거 호출을 다시 살리면 기존 연출이 복구된다.
    // Trigger_CombatLineBurst(Compute_CombatLineIntensity(combo));
}

void UI_PlayerHUD::Handle_BossSpawned(Shared<GameObject> obj)
{
    if (_cineTransitionState != ECineTransitionState::Idle && 
        _cineTransitionState != ECineTransitionState::FadeInToGame)
    {
        // 시네마틱이 끝난 후 바인딩 및 등장 연출을 위해 임시 저장
        _pendingBossObject = obj;
        return;
    }

    if (_bossHp)
        _bossHp->Bind_Boss(obj);

    if (_cineTransitionState != ECineTransitionState::Idle && 
        _cineTransitionState != ECineTransitionState::FadeInToGame)
    {
        return;
    }

    if (_bossGauge)
        _bossGauge->Set_Visibility(true);

    if (_bossIcon)
        _bossIcon->Set_Visibility(true);
}

void UI_PlayerHUD::Handle_CharacterDead(Shared<Character> deadCharacter, Shared<GameObject> damageCauser)
{
    auto deadEnemy = dynamic_pointer_cast<EnemyCharacter>(deadCharacter);
    if (!deadEnemy)
        return;

    if (deadEnemy->Get_ObjectType() == Protocol::OBJECT_TYPE_BOSS_PAIN)
    {
        GAME->Play_Sound(L"WinPanel.wav", ESoundChannel::UI, 0.6f);

        if (_bossLastFinish)
            _bossLastFinish->Set_Visibility(true);
    }

    auto causer = damageCauser;
    while (causer)
    {
        if (causer->Get_ObjectType() == Protocol::OBJECT_TYPE_PLAYER && causer->Is_Local())
        {
            Show_KOAnnounce();
            return;
        }

        auto owner = causer->Get_Owner();
        if (owner == causer)
            break;

        causer = owner;
    }
}

void UI_PlayerHUD::Handle_WireLockOnVisible(bool visible)
{
    if (visible)
        Show_WireLockOn();
    else
        Hide_WireLockOn();
}

void UI_PlayerHUD::Show_WireLockOn()
{
    if (!_wireLockOn)
        return;

    const float x = GAME->Get_UIReferenceWidth() * 0.5f;
    const float y = GAME->Get_UIReferenceHeight() * 0.5f;

    _wireLockOn->Show_LockOn(x, y);
}

void UI_PlayerHUD::Hide_WireLockOn()
{
    if (_wireLockOn)
        _wireLockOn->Hide_LockOn();
}

void UI_PlayerHUD::Set_MissionMarkerTarget(Shared<GameObject> targetObject)
{
    if (!_missionMarker)
        return;

    _missionMarker->Set_TargetObject(targetObject);
}

void UI_PlayerHUD::Clear_MissionMarkerTarget()
{
    if (!_missionMarker)
        return;

    _missionMarker->Clear_Target();
}

void UI_PlayerHUD::Show_MissionTitle(const wstring& text, float displayTime)
{
    if (!_missionTitleBanner)
        return;

    _missionTitleBanner->Set_LabelText(text);
    _missionTitleBanner->Set_UIOpacity(1.f);
    _missionTitleBanner->Set_Visibility(true);

    _missionTitleBaseX = _missionTitleBanner->Get_Transform()->Get_LocalPosition().x;
    _missionTitleTimer = displayTime;
}

void UI_PlayerHUD::Hide_MissionTitle()
{
    if (!_missionTitleBanner)
        return;

    _missionTitleBanner->Set_Visibility(false);
    _missionTitleTimer = 0.f;
}

HRESULT UI_PlayerHUD::Ready_MissionTitleUI()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    Background::FBackgroundDesc desc{};
    desc.posX = uiRefWidth * 0.5f;  // 덼포트 수평 정중앙
    desc.posY = uiRefHeight * 0.10f; // 기존 0.18에서 0.10으로 더 위로 이동
    desc.sizeX = 1000.f;
    desc.sizeY = 80.f;
    desc.zOrder = 0.75f;
    desc.levelIndex = _levelIndex;
    desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MISSION;
    desc.textureIndex = 5;
    desc.shaderPassIndex = 1;

    desc.textDesc.text = L"적을 쓰러뜨려라!";
    desc.textDesc.offset = Vec2(0.f, 0.f);
    desc.textDesc.size = Vec2(900.f, 60.f);
    desc.textDesc.style.fontSize = 28.f;
    desc.textDesc.style.color = Vec4(1.f, 1.f, 1.f, 1.f);
    desc.textDesc.style.hAlign = ETextHAlign::Center;
    desc.textDesc.style.vAlign = ETextVAlign::Middle;

    _missionTitleBanner = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND, EUILayer::Overlay, &desc);
    CHECK_NULL(_missionTitleBanner, E_FAIL);
    _missionTitleBanner->Set_Visibility(false);
    _missionTitleTimer = 0.f;

    return S_OK;
}


void UI_PlayerHUD::Update_MissionTitle(float timeDelta)
{
    if (_missionTitleTimer <= 0.f)
        return;

    _missionTitleTimer -= timeDelta;

    if (_missionTitleTimer <= MISSION_TITLE_FADE_TIME)
    {
        const float ratio = max(0.f, _missionTitleTimer / MISSION_TITLE_FADE_TIME);
        const float slideX = _missionTitleBaseX - (MISSION_TITLE_SLIDE_DIST * (1.f - ratio));

        _missionTitleBanner->Set_UIPosition(slideX, _missionTitleBanner->Get_Transform()->Get_LocalPosition().y);
        _missionTitleBanner->Set_UIOpacity(ratio);
    }

    if (_missionTitleTimer <= 0.f)
        Hide_MissionTitle();
}

void UI_PlayerHUD::Show_MissionEnd()
{
    if (!_missionEndBanner)
        return;

    GAME->Play_Sound(L"WinPanel.wav", ESoundChannel::UI, 0.6f);

    _missionEndBanner->Set_Visibility(true);
    _missionEndBanner->Set_UIOpacity(1.f);
}

void UI_PlayerHUD::Hide_MissionEnd()
{
    if (!_missionEndBanner)
        return;

    _missionEndBanner->Set_UIOpacity(0.f);
    _missionEndBanner->Set_Visibility(false);
}

void UI_PlayerHUD::Set_MissionEndOpacity(float alpha)
{
    if (!_missionEndBanner)
        return;

    const float clampedAlpha = clamp(alpha, 0.f, 1.f);
    _missionEndBanner->Set_Visibility(clampedAlpha > 0.f);
    _missionEndBanner->Set_UIOpacity(clampedAlpha);
}

void UI_PlayerHUD::Set_ScreenFadeAlpha(float alpha)
{
    if (!_screenFadePanel)
        return;

    _screenFadePanel->Set_FadeAlpha(alpha);
}

void UI_PlayerHUD::Set_HUDVisibilityForCinematic(bool isVisible)
{
    if (_status) _status->Set_Visibility(isVisible);
    if (_skillPanel) _skillPanel->Set_Visibility(isVisible);
    if (_timer) _timer->Set_Visibility(isVisible);
    if (_targeting) _targeting->Set_Visibility(isVisible);
    
    if (_bossHp) _bossHp->Set_Visibility(isVisible);
    if (_bossGauge) _bossGauge->Set_Visibility(isVisible);
    if (_bossIcon) _bossIcon->Set_Visibility(isVisible);
}

void UI_PlayerHUD::On_CinematicFinished()
{
    if (_cineTransitionState == ECineTransitionState::Playing)
    {
        // 시네마틱 종료 이벤트가 들어온 즉시 레터박스를 먼저 숨겨서
        // HUD가 복귀하는 페이드 구간에 위/아래 검은 띠가 남아 보이지 않게 한다.
        if (_letterBoxTop) _letterBoxTop->Set_FadeAlpha(0.f);
        if (_letterBoxBottom) _letterBoxBottom->Set_FadeAlpha(0.f);

        // 레벨 진입 시네마틱은 HUD를 다시 보여준다.
        // 보스 시네마틱도 동일하지만, 보스 HP 바인딩 로직은 보스 진입에만 적용한다.
        Set_HUDVisibilityForCinematic(true);
        
        if (!_isLevelEntryCinematic)
        {
            // 시네마틱이 끝나고 원래 화면으로 돌아올 때 보스 HP 바인딩 & 인트로 시작
            if (auto boss = _pendingBossObject.lock())
            {
                if (_bossHp) _bossHp->Bind_Boss(boss);
                if (_bossGauge) _bossGauge->Set_Visibility(true);
                if (_bossIcon) _bossIcon->Set_Visibility(true);
                _pendingBossObject.reset();
            }
        }

        if (_bossCineText)
            _bossCineText->Set_Visibility(false);

        if (_screenFadePanel)
            _screenFadePanel->Set_FadeAlpha(1.f);
            
        _cineTransitionState = ECineTransitionState::FadeInToGame;
        _cineTransitionTimer = 0.f;
    }
}

void UI_PlayerHUD::Trigger_LevelEntryCinematic(
    const string& cinematicTag,
    const wstring& introSoundFile,
    const wstring& gameplayBgmFile)
{
    // 이미 시네마틱이 진행 중이면 무시한다.
    if (_cineTransitionState != ECineTransitionState::Idle)
        return;

    GAME->Set_GameInputEnabled(false);
    Set_HUDVisibilityForCinematic(false);
    // 레벨 진입 시네마틱이 시작되는 동안에는 기존 BGM이나 이전 레벨에서 남은 BGM이 끼어들지 않도록 먼저 정리한다.
    GAME->Stop_SoundChannel(ESoundChannel::BGM, 0.f);

    GAME->Set_CinematicTransition(true);
    _isLevelEntryCinematic = true;
    _isCineDialoguePlayed = true; // 레벨 진입 시네마틱은 다이얼로그 없음
    _pendingCinematicTag = cinematicTag;
    _pendingLevelEntryIntroSoundFile = introSoundFile;
    _pendingLevelEntryGameplayBgmFile = gameplayBgmFile;
    _battleStartTimer = -1.f;
    _cineTransitionState = ECineTransitionState::FadeOutToBlack;
    _cineTransitionTimer = 0.f;
}

void UI_PlayerHUD::Handle_WaveStarted(const string& waveTag)
{
    // 웨이브가 실제로 시작된 프레임에 플레이어에게 미션 경보음을 먼저 들려준다.
    // 로컬 트리거/서버 authoritative 시작 모두 OnWaveStarted로 합류하므로 여기서 공통 처리한다.
    GAME->Play_Sound(L"MissionAlert.wav", ESoundChannel::UI, 0.6f);

    // 일반 웨이브는 레벨 종류나 웨이브 태그와 상관없이 공용 미션 고지를 띄운다.
    // 기존처럼 특정 레벨의 특정 태그에만 묶어두면 코노하/추가 웨이브에서 UI가 빠질 수 있다.
    if (waveTag != "BossPainWave" && waveTag != "Wave_Boss")
        Show_MissionTitle(L"적을 쓰러뜨려라!", 2.f);

    if (waveTag == "BossPainWave" || waveTag == "Wave_Boss")
    {
        GAME->Set_GameInputEnabled(false);
        Set_HUDVisibilityForCinematic(false);

        GAME->Play_BGM(L"BGM/Pain_BGM.mp3", 0.25f, true, 1.0f);
        
        GAME->Set_CinematicTransition(true); // 에디터 강제 개입 방지용
        _cineTransitionState = ECineTransitionState::FadeOutToBlack;
        _cineTransitionTimer = 0.f;
        _pendingCinematicTag = "Boss_Entry";
        _isCineDialoguePlayed = false;
    }
}

HRESULT UI_PlayerHUD::Ready_CinematicTransition()
{
    return S_OK;
}

void UI_PlayerHUD::Update_CinematicTransition(float timeDelta)
{
    if (_cineTransitionState == ECineTransitionState::Idle)
        return;
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();
    _cineTransitionTimer += timeDelta;
    switch (_cineTransitionState)
    {
    case ECineTransitionState::FadeOutToBlack:
    {
        const float ratio = std::clamp(_cineTransitionTimer / CINE_FADE_OUT_TIME, 0.f, 1.f);
        if (_screenFadePanel) _screenFadePanel->Set_FadeAlpha(ratio); 

        if (ratio >= 1.f)
        {
            if (_isLevelEntryCinematic && !_pendingLevelEntryIntroSoundFile.empty())
                GAME->Play_Sound(_pendingLevelEntryIntroSoundFile, ESoundChannel::BGM, 0.35f);

            const bool cinePlayed = GAME->Play_Cinematic(Utils::ToWString(_pendingCinematicTag), nullptr, true);

            // 레벨 진입 시네마틱이 아닐 때만(보스 진입) 보스 텍스트를 표시한다.
            if (!_isLevelEntryCinematic && _bossCineText)
                _bossCineText->Set_Visibility(true);

            if (_letterBoxTop) _letterBoxTop->Set_FadeAlpha(1.f);
            if (_letterBoxBottom) _letterBoxBottom->Set_FadeAlpha(1.f);

            // Play_Cinematic 실패 시(파일 없음) FadeInToCine을 건너뛰고 바로 Playing으로 전환
            if (cinePlayed)
                _cineTransitionState = ECineTransitionState::FadeInToCine;
            else
                _cineTransitionState = ECineTransitionState::Playing;

            _cineTransitionTimer = 0.f;
        }
        break;
    }
    case ECineTransitionState::FadeInToCine:
    {
        const float ratio = std::clamp(_cineTransitionTimer / CINE_FADE_OUT_TIME, 0.f, 1.f);
        if (_screenFadePanel) _screenFadePanel->Set_FadeAlpha(1.f - ratio); 

        if (ratio >= 1.f)
        {
            _cineTransitionState = ECineTransitionState::Playing;
            _cineTransitionTimer = 0.f;
        }
        break;
    }
    case ECineTransitionState::Playing:
    {
        if (!_isCineDialoguePlayed && _cineTransitionTimer >= 1.0f)
        {
            GAME->Play_Sound(L"BGM/Pain_Dialogue.wav", ESoundChannel::Voice, 1.0f);
            _isCineDialoguePlayed = true;
        }
        break;
    }
    case ECineTransitionState::FadeInToGame:
    {
        const float ratio = std::clamp(_cineTransitionTimer / CINE_FADE_OUT_TIME, 0.f, 1.f);
        if (_screenFadePanel) _screenFadePanel->Set_FadeAlpha(1.f - ratio);
        if (_letterBoxTop) _letterBoxTop->Set_FadeAlpha(0.f);
        if (_letterBoxBottom) _letterBoxBottom->Set_FadeAlpha(0.f);

        if (ratio >= 1.f)
        {
            GAME->Set_CinematicTransition(false); // 시네마틱 트랜지션 완전 종료
            GAME->Set_GameInputEnabled(true);
            
            if (_letterBoxTop) _letterBoxTop->Set_FadeAlpha(0.f);
            if (_letterBoxBottom) _letterBoxBottom->Set_FadeAlpha(0.f);

            // 코노하 레벨 진입 시네마틱이었다면 전투 개시 연출을 시작한다.
            if (_isLevelEntryCinematic)
                Show_BattleStartAnnounce();

            _isLevelEntryCinematic = false; // 레벨 진입 시네마틱 플래그 리셋
            _cineTransitionState = ECineTransitionState::Idle;
            _cineTransitionTimer = 0.f;
        }
        break;
    }
    }
}

HRESULT UI_PlayerHUD::Ready_CombatLines()
{
    Reset_CombatLineLayout();

    for (size_t i = 0; i < _combatLineLayers.size(); ++i)
    {
        auto& layer = _combatLineLayers[i];

        Background::FBackgroundDesc desc{};
        desc.posX = layer.basePosX;
        desc.posY = layer.basePosY;
        desc.sizeX = layer.baseSizeX;
        desc.sizeY = layer.baseSizeY;
        desc.zOrder = _zOrder + 0.007f + (static_cast<float>(i) * 0.0001f);
        desc.levelIndex = _levelIndex;
        desc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_SWORD_TRAIL;
        desc.textureIndex = layer.textureIndex;
        desc.shaderPassIndex = 7;

        layer.widget = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::Overlay,
            &desc);
        CHECK_NULL(layer.widget, E_FAIL);

        layer.widget->Set_UIRotationZ(layer.baseRotation);
        layer.widget->Set_UIOpacity(0.f);
        layer.widget->Set_Visibility(false);
    }

    return S_OK;
}

void UI_PlayerHUD::Reset_CombatLineLayout()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    _combatLineLayers.clear();
    _combatLineLayers.reserve(4);

    {
        FCombatLineLayer layer{};
        layer.textureIndex = COMBAT_LINE_TEXTURE_INDEX_07;
        layer.basePosX = 120.f;
        layer.basePosY = uiRefHeight * 0.24f;
        layer.baseSizeX = 430.f;
        layer.baseSizeY = 250.f;
        layer.baseRotation = 64.f;
        layer.alphaWeight = 0.78f;
        layer.scaleJitter = 0.012f;
        layer.phaseOffset = 0.f;
        _combatLineLayers.push_back(layer);
    }

    {
        FCombatLineLayer layer{};
        layer.textureIndex = COMBAT_LINE_TEXTURE_INDEX_07;
        layer.basePosX = uiRefWidth - 120.f;
        layer.basePosY = uiRefHeight * 0.24f;
        layer.baseSizeX = 430.f;
        layer.baseSizeY = 250.f;
        layer.baseRotation = -64.f;
        layer.alphaWeight = 0.78f;
        layer.scaleJitter = 0.012f;
        layer.phaseOffset = 0.85f;
        _combatLineLayers.push_back(layer);
    }

    {
        FCombatLineLayer layer{};
        layer.textureIndex = COMBAT_LINE_TEXTURE_INDEX_07;
        layer.basePosX = 120.f;
        layer.basePosY = uiRefHeight * 0.72f;
        layer.baseSizeX = 320.f;
        layer.baseSizeY = 180.f;
        layer.baseRotation = 112.f;
        layer.alphaWeight = 0.34f;
        layer.scaleJitter = 0.008f;
        layer.phaseOffset = 1.7f;
        _combatLineLayers.push_back(layer);
    }

    {
        FCombatLineLayer layer{};
        layer.textureIndex = COMBAT_LINE_TEXTURE_INDEX_07;
        layer.basePosX = uiRefWidth - 120.f;
        layer.basePosY = uiRefHeight * 0.72f;
        layer.baseSizeX = 320.f;
        layer.baseSizeY = 180.f;
        layer.baseRotation = -112.f;
        layer.alphaWeight = 0.34f;
        layer.scaleJitter = 0.008f;
        layer.phaseOffset = 2.55f;
        _combatLineLayers.push_back(layer);
    }
}

void UI_PlayerHUD::Update_CombatLineBurst(float timeDelta)
{
    if (!_isCombatLineBurstActive)
        return;

    _combatLinePhaseTime += timeDelta;

    if (_combatLineHoldTimer > 0.f)
    {
        _combatLineHoldTimer = max(0.f, _combatLineHoldTimer - timeDelta);
    }
    else
    {
        _combatLineFadeTimer = max(0.f, _combatLineFadeTimer - timeDelta);
    }

    const float alpha = Compute_CombatLineAlpha();

    if (_combatLineHoldTimer <= 0.f && _combatLineFadeTimer <= 0.f)
    {
        Stop_CombatLineBurst();
        return;
    }

    for (auto& layer : _combatLineLayers)
    {
        if (!layer.widget)
            continue;

        const float phase = (_combatLinePhaseTime * 10.f) + layer.phaseOffset;
        const float jitterScale = 1.f + (sinf(phase) * layer.scaleJitter * _combatLineIntensity);

        layer.widget->Set_Visibility(true);
        layer.widget->Set_UIPosition(layer.basePosX, layer.basePosY);
        layer.widget->Set_UIRotationZ(layer.baseRotation);
        layer.widget->Set_UIScale(layer.baseSizeX * jitterScale, layer.baseSizeY * jitterScale);
        layer.widget->Set_UIOpacity(alpha * layer.alphaWeight);
    }
}

float UI_PlayerHUD::Compute_CombatLineAlpha() const
{
    if (_combatLineHoldTimer > 0.f)
        return 1.f;

    if (_combatLineFadeTimer <= 0.f)
        return 0.f;

    const float ratio = _combatLineFadeTimer / COMBAT_LINE_FADE_TIME;
    return clamp(ratio, 0.f, 1.f);
}

float UI_PlayerHUD::Compute_CombatLineIntensity(uint32 combo) const
{
    const uint32 safeCombo = min<uint32>(combo, 6);
    const float intensity = 1.f + ((static_cast<float>(safeCombo) - 1.f) * 0.05f);
    return clamp(intensity, 1.f, 1.3f);
}

void UI_PlayerHUD::Trigger_CombatLineBurst(float intensity)
{
    _isCombatLineBurstActive = true;
    _combatLineHoldTimer = COMBAT_LINE_HOLD_TIME;
    _combatLineFadeTimer = COMBAT_LINE_FADE_TIME;
    _combatLinePhaseTime = 0.f;
    _combatLineIntensity = intensity;

    for (auto& layer : _combatLineLayers)
    {
        if (!layer.widget)
            continue;

        layer.widget->Set_Visibility(true);
    }
}

void UI_PlayerHUD::Stop_CombatLineBurst()
{
    _isCombatLineBurstActive = false;
    _combatLineHoldTimer = 0.f;
    _combatLineFadeTimer = 0.f;
    _combatLinePhaseTime = 0.f;
    _combatLineIntensity = 1.f;

    for (auto& layer : _combatLineLayers)
    {
        if (!layer.widget)
            continue;

        layer.widget->Set_UIOpacity(0.f);
        layer.widget->Set_Visibility(false);
    }
}

HRESULT UI_PlayerHUD::Ready_KOAnnounce()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();
    const float announceX = (uiRefWidth * 0.5f) + _koAnnounceOffsetX;
    const float announceY = (uiRefHeight * 0.30f) + _koAnnounceOffsetY;

    Background::FBackgroundDesc koBgDesc{};
    koBgDesc.name = L"KO_BG";
    koBgDesc.posX = announceX;
    koBgDesc.posY = announceY;
    koBgDesc.sizeX = KO_BG_BASE_WIDTH;
    koBgDesc.sizeY = KO_BG_BASE_HEIGHT;
    koBgDesc.zOrder = _zOrder + 0.30f;
    koBgDesc.levelIndex = _levelIndex;
    koBgDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_KO;
    koBgDesc.textureIndex = 1;
    koBgDesc.shaderPassIndex = 1;

    _koBackground = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND,
        EUILayer::Overlay,
        &koBgDesc);
    CHECK_NULL(_koBackground, E_FAIL);

    Background::FBackgroundDesc koTextDesc{};
    koTextDesc.name = L"KO_Text";
    koTextDesc.posX = announceX;
    koTextDesc.posY = announceY;
    koTextDesc.sizeX = KO_TEXT_BASE_WIDTH;
    koTextDesc.sizeY = KO_TEXT_BASE_HEIGHT;
    koTextDesc.zOrder = _zOrder + 0.31f;
    koTextDesc.levelIndex = _levelIndex;
    koTextDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_KO;
    koTextDesc.textureIndex = 0;
    koTextDesc.shaderPassIndex = 1;

    _koText = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND,
        EUILayer::Overlay,
        &koTextDesc);
    CHECK_NULL(_koText, E_FAIL);

    Hide_KOAnnounce();

    return S_OK;
}

void UI_PlayerHUD::Show_KOAnnounce()
{
    _isKOVisible = true;
    _koVisibleTimer = KO_ANNOUNCE_VISIBLE_TIME;

    if (_koBackground)
    {
        _koBackground->Set_Visibility(true);
        _koBackground->Set_UIRotationZ(0.f);
        _koBackground->Set_UIScale(KO_BG_BASE_WIDTH, KO_BG_BASE_HEIGHT);
        _koBackground->Set_UIOpacity(0.f);
    }

    if (_koText)
    {
        _koText->Set_Visibility(false);
        _koText->Set_UIRotationZ(KO_ANNOUNCE_ROTATION_DEGREE);
        _koText->Set_UIScale(KO_TEXT_BASE_WIDTH * KO_ANNOUNCE_START_SCALE, KO_TEXT_BASE_HEIGHT * KO_ANNOUNCE_START_SCALE);
        _koText->Set_UIOpacity(0.f);
    }
}

void UI_PlayerHUD::Hide_KOAnnounce()
{
    _isKOVisible = false;
    _koVisibleTimer = 0.f;

    if (_koBackground)
    {
        _koBackground->Set_UIOpacity(0.f);
        _koBackground->Set_Visibility(false);
    }

    if (_koText)
    {
        _koText->Set_UIOpacity(0.f);
        _koText->Set_Visibility(false);
    }
}

void UI_PlayerHUD::Update_KOAnnounce(float timeDelta)
{
    if (!_isKOVisible)
        return;

    _koVisibleTimer = max(0.f, _koVisibleTimer - timeDelta);

    const float elapsedTime = KO_ANNOUNCE_VISIBLE_TIME - _koVisibleTimer;
    const float bgIntroRatio = (KO_BG_FADE_IN_TIME > FLT_EPSILON)
        ? elapsedTime / KO_BG_FADE_IN_TIME
        : 1.f;
    const float clampedBgIntroRatio = clamp(bgIntroRatio, 0.f, 1.f);

    const float textElapsedTime = elapsedTime - KO_TEXT_DELAY_TIME;
    const float textIntroRatio = (KO_ANNOUNCE_INTRO_TIME > FLT_EPSILON)
        ? textElapsedTime / KO_ANNOUNCE_INTRO_TIME
        : 1.f;
    const float clampedTextIntroRatio = clamp(textIntroRatio, 0.f, 1.f);
    const bool isTextVisible = textElapsedTime >= 0.f;

    const float fadeAlpha = (_koVisibleTimer < KO_ANNOUNCE_FADE_TIME)
        ? clamp(_koVisibleTimer / KO_ANNOUNCE_FADE_TIME, 0.f, 1.f)
        : 1.f;

    const float bgAlpha = clampedBgIntroRatio * fadeAlpha;
    const float textAlpha = isTextVisible ? fadeAlpha : 0.f;
    const float popScale = ::lerp(KO_ANNOUNCE_START_SCALE, 1.f, clampedTextIntroRatio);

    if (_koBackground)
    {
        _koBackground->Set_UIRotationZ(0.f);
        _koBackground->Set_UIScale(KO_BG_BASE_WIDTH, KO_BG_BASE_HEIGHT);
        _koBackground->Set_UIOpacity(bgAlpha);
    }

    if (_koText)
    {
        _koText->Set_Visibility(isTextVisible);
        _koText->Set_UIRotationZ(KO_ANNOUNCE_ROTATION_DEGREE);
        _koText->Set_UIScale(KO_TEXT_BASE_WIDTH * popScale, KO_TEXT_BASE_HEIGHT * popScale);
        _koText->Set_UIOpacity(textAlpha);
    }

    if (_koVisibleTimer <= 0.f)
        Hide_KOAnnounce();
}

void UI_PlayerHUD::Apply_AnnouncePositions()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();
    const float uiScale = GAME->Get_UIScale();
    const float safeScale = max(uiScale, FLT_EPSILON);

    if (_announceCombo)
    {
        const float comboX = (uiRefWidth * 0.55f) + (_comboAnnounceOffsetX / safeScale);
        const float comboY = (uiRefHeight * 0.5f) - 100.f + (_comboAnnounceOffsetY / safeScale);
        _announceCombo->Set_AnnouncePosition(comboX, comboY);
    }

    const float koX = (uiRefWidth * 0.5f) + (_koAnnounceOffsetX / safeScale);
    const float koY = (uiRefHeight * 0.30f) + (_koAnnounceOffsetY / safeScale);

    if (_koBackground)
        _koBackground->Set_UIPosition(koX, koY);

    if (_koText)
        _koText->Set_UIPosition(koX, koY);
}

HRESULT UI_PlayerHUD::Ready_UI(void* arg)
{
    UIObject::FUIDesc statDesc;
    statDesc.posX = 0.f;
    statDesc.posY = 0.f;
    statDesc.sizeX = 512.f;
    statDesc.sizeY = 128.f;
    statDesc.zOrder = _zOrder + 0.01f;
    statDesc.levelIndex = _levelIndex;

    _status = Create_Child<UI_PlayerStatus>(Protocol::OBJECT_TYPE_UI_PLAYER_STATUS, EUILayer::HUD, &statDesc);
    CHECK_NULL(_status, E_FAIL);

    _status->Get_Transform()->Set_LocalPosition(255.f, 945.f, _zOrder);

    UIObject::FUIDesc skillDesc;
    skillDesc.posX = 0.f;
    skillDesc.posY = 0.f;
    skillDesc.sizeX = 1.f;
    skillDesc.sizeY = 1.f;
    skillDesc.zOrder = _zOrder + 0.01f;
    skillDesc.levelIndex = _levelIndex;

    _skillPanel = Create_Child<UI_PlayerSkill>(Protocol::OBJECT_TYPE_UI_PLAYER_SKILL, EUILayer::HUD, &skillDesc);
    CHECK_NULL(_skillPanel, E_FAIL);

    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    _skillPanel->Get_Transform()->Set_LocalPosition(
        uiRefWidth - 220.f,
        uiRefHeight - 140.f,
        _zOrder);

    UIObject::FUIDesc announceDesc;
    announceDesc.posX = 0.f;
    announceDesc.posY = 0.f;
    announceDesc.sizeX = 1.f;
    announceDesc.sizeY = 1.f;
    announceDesc.zOrder = _zOrder + 0.01f;
    announceDesc.levelIndex = _levelIndex;

    _announceCombo = Create_Child<UI_AnnounceCombo>(Protocol::OBJECT_TYPE_UI_ANNOUNCE_COMBO, EUILayer::HUD, &announceDesc);
    CHECK_NULL(_announceCombo, E_FAIL);
    Apply_AnnouncePositions();

    UIObject::FUIDesc targetDesc;
    targetDesc.posX = 0.f;
    targetDesc.posY = 0.f;
    targetDesc.sizeX = 1.f;
    targetDesc.sizeY = 1.f;
    targetDesc.zOrder = _zOrder + 0.01f;
    targetDesc.levelIndex = _levelIndex;

    _targeting = Create_Child<UI_Targeting>(Protocol::OBJECT_TYPE_UI_TARGETING, EUILayer::Overlay, &targetDesc);
    CHECK_NULL(_targeting, E_FAIL);

    {
        UI_MissionMarker::FUIMissionMarkerDesc markerDesc;
        markerDesc.posX = 0.f;
        markerDesc.posY = 0.f;
        markerDesc.sizeX = 128.f + 64.f;
        markerDesc.sizeY = 128.f + 64.f;
        markerDesc.zOrder = _zOrder + 0.02f;
        markerDesc.levelIndex = _levelIndex;
        markerDesc.textureIndex = 0;

        _missionMarker = Create_Child<UI_MissionMarker>(
            Protocol::OBJECT_TYPE_UI_MISSION_MARKER,
            EUILayer::Overlay,
            &markerDesc);
        CHECK_NULL(_missionMarker, E_FAIL);
    }

    // 보스 체력
    {
        const float bossGaugeX = uiRefWidth * 0.5f;
        const float bossGaugeY = (uiRefHeight * 0.5f) - 400.f;

        Background::FBackgroundDesc gaugeDesc{};
        gaugeDesc.posX = bossGaugeX;
        gaugeDesc.posY = bossGaugeY;
        gaugeDesc.sizeX = 580.f;
        gaugeDesc.sizeY = 128.f;
        gaugeDesc.zOrder = _zOrder;
        gaugeDesc.levelIndex = _levelIndex;
        gaugeDesc.textureType = Protocol::COMPONENT_TYPE_BOSS_HP;
        gaugeDesc.textureIndex = 0;
        gaugeDesc.shaderPassIndex = 1;

        _bossGauge = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &gaugeDesc);
        CHECK_NULL(_bossGauge, E_FAIL);

        Background::FBackgroundDesc iconDesc{};
        iconDesc.posX = bossGaugeX - 198.4f;
        iconDesc.posY = bossGaugeY;
        iconDesc.sizeX = 126.83f;
        iconDesc.sizeY = 106.45f;
        iconDesc.zOrder = _zOrder + 0.01f;
        iconDesc.levelIndex = _levelIndex;
        iconDesc.textureType = Protocol::COMPONENT_TYPE_BOSS_HP;
        iconDesc.textureIndex = 2;
        iconDesc.shaderPassIndex = 1;

        _bossIcon = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &iconDesc);
        CHECK_NULL(_bossIcon, E_FAIL);

        Background::FBackgroundDesc cineTextDesc{};
        cineTextDesc.posX = 220.f; // 조금 더 왼쪽으로
        cineTextDesc.posY = 280.f; // 레터박스에 안 잘리게 아래로
        cineTextDesc.sizeX = 512.f; // 텍스트 크기에 맞게 조정
        cineTextDesc.sizeY = 512.f;
        cineTextDesc.zOrder = _zOrder + 0.02f;
        cineTextDesc.levelIndex = _levelIndex;
        cineTextDesc.textureType = Protocol::COMPONENT_TYPE_BOSS_HP;
        cineTextDesc.textureIndex = 3;
        cineTextDesc.shaderPassIndex = 1;

        _bossCineText = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::HUD,
            &cineTextDesc);
        CHECK_NULL(_bossCineText, E_FAIL);
        _bossCineText->Set_Visibility(false);

        UIObject::FUIDesc bossHpDesc{};
        bossHpDesc.posX = bossGaugeX + 36.5f;
        bossHpDesc.posY = bossGaugeY + 2.9f;
        bossHpDesc.sizeX = 635.f;
        bossHpDesc.sizeY = 82.49f;
        bossHpDesc.zOrder = _zOrder;
        bossHpDesc.levelIndex = _levelIndex;

        _bossHp = Create_Child<UI_BossHp>(
            Protocol::OBJECT_TYPE_UI_BOSS_HP,
            EUILayer::HUD,
            &bossHpDesc);
        CHECK_NULL(_bossHp, E_FAIL);

        _bossHp->Set_FillRange(98.f / 512.f, 413.f / 512.f);

        _bossGauge->Set_Visibility(false);
        _bossIcon->Set_Visibility(false);
        _bossHp->Set_Visibility(false);
    }

    CHECK_FAILED(Ready_CombatLines(), E_FAIL);
    CHECK_FAILED(Ready_KOAnnounce(), E_FAIL);
    CHECK_FAILED(Ready_Timer(), E_FAIL);
    CHECK_FAILED(Ready_WireLockOn(), E_FAIL);
    CHECK_FAILED(Ready_MissionClearUI(), E_FAIL);
    CHECK_FAILED(Ready_MissionTitleUI(), E_FAIL);
    CHECK_FAILED(Ready_CinematicTransition(), E_FAIL);

    return S_OK;
}

HRESULT UI_PlayerHUD::Ready_Timer()
{
    UI_Timer::FTimerDesc timerDesc{};

    timerDesc.posX = GAME->Get_UIReferenceWidth() - 210.f;
    timerDesc.posY = 72.f;
    timerDesc.sizeX = 1.f;
    timerDesc.sizeY = 1.f;
    timerDesc.zOrder = _zOrder + 0.20f;
    timerDesc.levelIndex = _levelIndex;
    timerDesc.startSeconds = 600.f;

    _timer = Create_Child<UI_Timer>(
        Protocol::OBJECT_TYPE_UI_TIMER,
        EUILayer::HUD,
        &timerDesc);

    return S_OK;
}

HRESULT UI_PlayerHUD::Ready_WireLockOn()
{
    UI_WireLockOn::FWireLockOnDesc desc{};

    desc.posX = GAME->Get_UIReferenceWidth() * 0.5f;
    desc.posY = GAME->Get_UIReferenceHeight() * 0.5f;
    desc.sizeX = 1.f;
    desc.sizeY = 1.f;
    desc.zOrder = _zOrder + 0.40f;
    desc.levelIndex = _levelIndex;

    _wireLockOn = Create_Child<UI_WireLockOn>(
        Protocol::OBJECT_TYPE_UI_LOCK_ON,
        EUILayer::Overlay,
        &desc);

    CHECK_NULL(_wireLockOn, E_FAIL);

    _wireLockOn->Hide_LockOn();

    return S_OK;
}

HRESULT UI_PlayerHUD::Ready_MissionClearUI()
{
    const float uiRefWidth = GAME->Get_UIReferenceWidth();
    const float uiRefHeight = GAME->Get_UIReferenceHeight();

    Background::FBackgroundDesc missionDesc{};
    missionDesc.posX = uiRefWidth * 0.5f;
    missionDesc.posY = uiRefHeight * 0.32f;
    missionDesc.sizeX = 1600.f;
    missionDesc.sizeY = 300.f;
    missionDesc.zOrder = 0.86f;
    missionDesc.levelIndex = _levelIndex;
    missionDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MISSION;
    missionDesc.textureIndex = 3;
    missionDesc.shaderPassIndex = 1;

    _missionEndBanner = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND,
        EUILayer::Overlay,
        &missionDesc);
    CHECK_NULL(_missionEndBanner, E_FAIL);

    _missionEndBanner->Set_Visibility(false);
    _missionEndBanner->Set_UIOpacity(0.f);

    // 전투 개시 고지 배너 (TEXTURE_MISSION 인덱스 1 - Mission Start)
    // 코노하 레벨 진입 시네마틱이 끝난 직후 스케일업 + 페이드아웃 연출에 사용한다.
    {
        Background::FBackgroundDesc battleDesc{};
        battleDesc.posX = uiRefWidth * 0.5f;
        battleDesc.posY = uiRefHeight * 0.5f;
        battleDesc.sizeX = 1344.f; // 1920 * 0.7
        battleDesc.sizeY = 266.f;  // 380 * 0.7
        battleDesc.zOrder = 0.88f;
        battleDesc.levelIndex = _levelIndex;
        battleDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MISSION;
        battleDesc.textureIndex = 1;   // Mission Start 이미지
        battleDesc.shaderPassIndex = 1;

        _battleStartBanner = Create_Child<Background>(
            Protocol::OBJECT_TYPE_BACKGROUND,
            EUILayer::Overlay,
            &battleDesc);
        CHECK_NULL(_battleStartBanner, E_FAIL);

        _battleStartBanner->Set_Visibility(false);
        _battleStartBanner->Set_UIOpacity(0.f);
    }

    Background::FBackgroundDesc lastFinishDesc{};
    lastFinishDesc.posX = uiRefWidth * 0.5f;
    lastFinishDesc.posY = uiRefHeight * 0.5f;
    lastFinishDesc.sizeX = 1637.f;
    lastFinishDesc.sizeY = 960.f;
    lastFinishDesc.zOrder = 0.96f; // 페이드나 다른 오버레이보다 위에 오게
    lastFinishDesc.levelIndex = _levelIndex;
    lastFinishDesc.textureType = Protocol::COMPONENT_TYPE_TEXTURE_MISSION;
    lastFinishDesc.textureIndex = 4;
    lastFinishDesc.shaderPassIndex = 1;

    _bossLastFinish = Create_Child<Background>(
        Protocol::OBJECT_TYPE_BACKGROUND,
        EUILayer::Overlay,
        &lastFinishDesc);
    CHECK_NULL(_bossLastFinish, E_FAIL);
    _bossLastFinish->Set_Visibility(false);

    UI_ScreenFade::FScreenFadeDesc fadeDesc{};
    fadeDesc.posX = uiRefWidth * 0.5f;
    fadeDesc.posY = uiRefHeight * 0.5f;
    fadeDesc.sizeX = uiRefWidth;
    fadeDesc.sizeY = uiRefHeight;
    fadeDesc.zOrder = 0.95f;
    fadeDesc.levelIndex = _levelIndex;
    fadeDesc.fadeColor = Color(0.f, 0.f, 0.f, 1.f);
    fadeDesc.initialAlpha = 0.f;

    _screenFadePanel = Create_Child<UI_ScreenFade>(
        Protocol::OBJECT_TYPE_UI_SCREEN_FADE,
        EUILayer::Overlay,
        &fadeDesc);

    CHECK_NULL(_screenFadePanel, E_FAIL);

    UI_ScreenFade::FScreenFadeDesc letterBoxTopDesc{};
    letterBoxTopDesc.posX = uiRefWidth * 0.5f;
    letterBoxTopDesc.posY = 75.f; // 150 높이의 절반
    letterBoxTopDesc.sizeX = uiRefWidth;
    letterBoxTopDesc.sizeY = 150.f;
    letterBoxTopDesc.zOrder = 0.94f; // 스크린 페이드보다는 아래
    letterBoxTopDesc.levelIndex = _levelIndex;
    letterBoxTopDesc.fadeColor = Color(0.f, 0.f, 0.f, 1.f);
    letterBoxTopDesc.initialAlpha = 0.f;

    _letterBoxTop = Create_Child<UI_ScreenFade>(
        Protocol::OBJECT_TYPE_UI_SCREEN_FADE,
        EUILayer::Overlay,
        &letterBoxTopDesc);
    CHECK_NULL(_letterBoxTop, E_FAIL);

    UI_ScreenFade::FScreenFadeDesc letterBoxBottomDesc{};
    letterBoxBottomDesc.posX = uiRefWidth * 0.5f;
    letterBoxBottomDesc.posY = uiRefHeight - 75.f;
    letterBoxBottomDesc.sizeX = uiRefWidth;
    letterBoxBottomDesc.sizeY = 150.f;
    letterBoxBottomDesc.zOrder = 0.94f;
    letterBoxBottomDesc.levelIndex = _levelIndex;
    letterBoxBottomDesc.fadeColor = Color(0.f, 0.f, 0.f, 1.f);
    letterBoxBottomDesc.initialAlpha = 0.f;

    _letterBoxBottom = Create_Child<UI_ScreenFade>(
        Protocol::OBJECT_TYPE_UI_SCREEN_FADE,
        EUILayer::Overlay,
        &letterBoxBottomDesc);
    CHECK_NULL(_letterBoxBottom, E_FAIL);

    return S_OK;
}

Shared<UI_PlayerHUD> UI_PlayerHUD::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<UI_PlayerHUD>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create Prototype : PlayerHUD");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> UI_PlayerHUD::Clone(void* arg)
{
    auto clone = make_shared<UI_PlayerHUD>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : UI_PlayerHUD");

        return nullptr;
    }

    return clone;
}

void UI_PlayerHUD::Free()
{
    auto& hub = GAME->Get_DelegateHub();

    if (_remotePlayerSpawnedHandle.IsValid())
    {
        hub.OnRemotePlayerObjectSpawned.Remove(_remotePlayerSpawnedHandle);
        _remotePlayerSpawnedHandle.Reset();
    }

    if (_comboHitHandle.IsValid())
    {
        hub.OnPlayerComboHit.Remove(_comboHitHandle);
        _comboHitHandle.Reset();
    }

    if (_bossObjectSpawnedHandle.IsValid())
    {
        hub.OnBossObjectSpawned.Remove(_bossObjectSpawnedHandle);
        _bossObjectSpawnedHandle.Reset();
    }

    if (_deadHandle.IsValid())
    {
        hub.OnDead.Remove(_deadHandle);
        _deadHandle.Reset();
    }

    if (_wireLockOnVisibleHandle.IsValid())
    {
        hub.OnWireLockOnVisible.Remove(_wireLockOnVisibleHandle);
        _wireLockOnVisibleHandle.Reset();
    }

    if (_missionMarkerTargetHandle.IsValid())
    {
        hub.OnMissionMarkerTargetChanged.Remove(_missionMarkerTargetHandle);
        _missionMarkerTargetHandle.Reset();
    }

    if (_missionMarkerClearHandle.IsValid())
    {
        hub.OnMissionMarkerTargetCleared.Remove(_missionMarkerClearHandle);
        _missionMarkerClearHandle.Reset();
    }

    if (_waveStartedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnWaveStarted.Remove(_waveStartedHandle);
        _waveStartedHandle.Reset();
    }
    
    if (_cineFinishedHandle.IsValid())
    {
        GAME->Get_DelegateHub().OnCinematicFinished.Remove(_cineFinishedHandle);
        _cineFinishedHandle.Reset();
    }

    HUD::Free();
}

void UI_PlayerHUD::Show_BattleStartAnnounce()
{
    if (!_battleStartBanner)
        return;

    if (!_pendingLevelEntryGameplayBgmFile.empty())
    {
        GAME->Play_BGM(_pendingLevelEntryGameplayBgmFile, 0.2f, true, 0.45f);
        _pendingLevelEntryGameplayBgmFile.clear();
    }

    // Set_UIScale은 배율이 아니라 UIReferenceWidth 기준 픽셀 크기입니다. 1920x380의 0.7배율 크기로 리셋합니다.
    _battleStartBanner->Set_UIScale(1344.f, 266.f);
    _battleStartBanner->Set_UIOpacity(1.f);
    _battleStartBanner->Set_Visibility(true);

    _battleStartTimer = 0.f; // 0 이상 = 활성
}

void UI_PlayerHUD::Update_BattleStartAnnounce(float timeDelta)
{
    // 음수이면 비활성 상태이므로 무시한다.
    if (_battleStartTimer < 0.f || !_battleStartBanner)
        return;

    _battleStartTimer += timeDelta;

    const float totalTime = BATTLE_START_HOLD_TIME + BATTLE_START_ZOOM_TIME;

    if (_battleStartTimer >= totalTime)
    {
        // 연출 완료 → 숨기고 비활성화
        _battleStartBanner->Set_Visibility(false);
        _battleStartBanner->Set_UIOpacity(0.f);
        _battleStartTimer = -1.f;
        return;
    }

    if (_battleStartTimer < BATTLE_START_HOLD_TIME)
    {
        // Hold 구간: 기본 크기(1344x266), 완전 불투명
        _battleStartBanner->Set_UIScale(1344.f, 266.f);
        _battleStartBanner->Set_UIOpacity(1.f);
    }
    else
    {
        // Zoom + FadeOut 구간
        const float zoomElapsed = _battleStartTimer - BATTLE_START_HOLD_TIME;
        const float ratio = std::clamp(zoomElapsed / BATTLE_START_ZOOM_TIME, 0.f, 1.f);

        // 스케일: 기본 크기 -> 기본 크기 * BATTLE_START_SCALE_END (선형 보간)
        const float scaleMultiplier = 1.f + (BATTLE_START_SCALE_END - 1.f) * ratio;
        const float currentSizeX = 1344.f * scaleMultiplier;
        const float currentSizeY = 266.f * scaleMultiplier;
        
        _battleStartBanner->Set_UIScale(currentSizeX, currentSizeY);

        // 투명도: 1.0 → 0.0 (선형 감소)
        _battleStartBanner->Set_UIOpacity(1.f - ratio);
    }
}
