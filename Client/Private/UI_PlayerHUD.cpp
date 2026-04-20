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
#include "GameInstance.h"

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

    GAME->Get_DelegateHub().OnRemotePlayerObjectSpawned.Add(
        this, &UI_PlayerHUD::Handle_RemotePlayerObjectSpawned);

    return S_OK;
}

void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);

    Update_RemotePlayerStatusList();
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

    // 콤보 UI
    UIObject::FUIDesc announceDesc;
    announceDesc.posX = 0.f;
    announceDesc.posY = 0.f;
    announceDesc.sizeX = 1.f;
    announceDesc.sizeY = 1.f;
    announceDesc.zOrder = _zOrder + 0.01f;
    announceDesc.levelIndex = _levelIndex;

    _announceCombo = Create_Child<UI_AnnounceCombo>(Protocol::OBJECT_TYPE_UI_ANNOUNCE_COMBO, EUILayer::HUD, &announceDesc);
    CHECK_NULL(_announceCombo, E_FAIL);

    // 타겟팅 UI
    UIObject::FUIDesc targetDesc;
    targetDesc.posX = 0.f;
    targetDesc.posY = 0.f;
    targetDesc.sizeX = 1.f;
    targetDesc.sizeY = 1.f;
    targetDesc.zOrder = _zOrder + 0.01f;
    targetDesc.levelIndex = _levelIndex;

    _targeting = Create_Child<UI_Targeting>(Protocol::OBJECT_TYPE_UI_TARGETING, EUILayer::Overlay, &targetDesc);
    CHECK_NULL(_targeting, E_FAIL);

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
    HUD::Free();
}
