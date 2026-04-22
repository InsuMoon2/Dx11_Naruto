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
#include "GameInstance.h"
#include "UI_BossHp.h"

REGISTER_GAMEOBJECT(UI_PlayerHUD, Protocol::OBJECT_TYPE_UI_PLAYER_HUD)

UI_PlayerHUD::UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : HUD(device, context)
{

}

UI_PlayerHUD::UI_PlayerHUD(const UI_PlayerHUD& rhs)
    : HUD(rhs)
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

    return S_OK;
}

void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);

    Update_RemotePlayerStatusList();
    Update_CombatLineBurst(timeDelta);

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

    UI_PlayerHP::FPlayerHPDesc hpDesc{};
    hpDesc.posX = 0.f;
    hpDesc.posY = 0.f;
    hpDesc.sizeX = 220.f;
    hpDesc.sizeY = 34.f;
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
        entry.hpBar->Get_Transform()->Set_LocalScale(220.f, 34.f, 1.f);
    }

    UI_Text::FUITextDesc nameDesc{};
    nameDesc.posX = 0.f;
    nameDesc.posY = 0.f;
    nameDesc.sizeX = 220.f;
    nameDesc.sizeY = 28.f;
    nameDesc.zOrder = _zOrder + 0.05f;
    nameDesc.levelIndex = _levelIndex;
    nameDesc.text = player->Get_PlayerName().empty() ? L"Player" : player->Get_PlayerName();
    nameDesc.style.fontSize = 18.f;
    nameDesc.style.color = Color(1.f, 1.f, 1.f, 1.f);
    nameDesc.style.hAlign = ETextHAlign::Left;
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
                    if (entry.hpBar)
                        entry.hpBar->Set_Destroy(true);

                    if (entry.nameText)
                        entry.nameText->Set_Destroy(true);
                }

                return shouldRemove;
            }),
        _remotePlayerStatuses.end());

    const float baseX = 40.f;
    const float baseY = 120.f;
    const float gapY = 58.f;

    for (size_t i = 0; i < _remotePlayerStatuses.size(); ++i)
    {
        auto& entry = _remotePlayerStatuses[i];

        const float y = baseY + static_cast<float>(i) * gapY;

        if (auto hpBar = entry.hpBar)
        {
            hpBar->Get_Transform()->Set_LocalPosition(baseX + 110.f, y + 24.f, _zOrder + 0.04f);

            auto combat = entry.combat.lock();
            hpBar->Set_Ratio(combat ? combat->Get_HpRatio() : 1.f);
        }

        if (auto nameText = entry.nameText)
        {
            auto player = entry.player.lock();
            if (player)
                nameText->Set_Text(player->Get_PlayerName());

            nameText->Get_Transform()->Set_LocalPosition(baseX + 110.f, y, _zOrder + 0.05f);
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
    Trigger_CombatLineBurst(Compute_CombatLineIntensity(combo));
}

void UI_PlayerHUD::Handle_BossSpawned(Shared<GameObject> obj)
{
    if (_bossHp)
        _bossHp->Bind_Boss(obj);

    if (_bossGauge)
        _bossGauge->Set_Visibility(true);

    if (_bossIcon)
        _bossIcon->Set_Visibility(true);
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

    UIObject::FUIDesc targetDesc;
    targetDesc.posX = 0.f;
    targetDesc.posY = 0.f;
    targetDesc.sizeX = 1.f;
    targetDesc.sizeY = 1.f;
    targetDesc.zOrder = _zOrder + 0.01f;
    targetDesc.levelIndex = _levelIndex;

    _targeting = Create_Child<UI_Targeting>(Protocol::OBJECT_TYPE_UI_TARGETING, EUILayer::Overlay, &targetDesc);
    CHECK_NULL(_targeting, E_FAIL);

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

    HUD::Free();
}
