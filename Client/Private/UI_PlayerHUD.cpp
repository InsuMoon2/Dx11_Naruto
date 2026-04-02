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

    return S_OK;
}

void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);
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
