#include "pch.h"
#include "UI_PlayerHUD.h"
#include "GameInstance.h"
#include "UI_PlayerStatus.h"
#include "UI_PlayerSkill.h"

UI_PlayerHUD::UI_PlayerHUD(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : HUD(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_UI_PLAYER_HUD);
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
}

HRESULT UI_PlayerHUD::Ready_UI(void* arg)
{
    UIObject::FUIDesc statDesc;
    statDesc.posX = 0.f;
    statDesc.posY = 0.f;
    statDesc.sizeX = 500.f;
    statDesc.sizeY = 160.f;
    statDesc.zOrder = _zOrder + 0.01f;
    statDesc.levelIndex = _levelIndex;

    _status = Create_Child<UI_PlayerStatus>(Protocol::OBJECT_TYPE_UI_PLAYER_STATUS, EUILayer::HUD, &statDesc);
    CHECK_NULL(_status, E_FAIL);

    _status->Get_Transform()->Set_LocalPosition(255.f, 750.f, _zOrder);

    UIObject::FUIDesc skillDesc;
    skillDesc.posX = 0.f;
    skillDesc.posY = 0.f;
    skillDesc.sizeX = 1.f;
    skillDesc.sizeY = 1.f;
    skillDesc.zOrder = _zOrder + 0.01f;
    skillDesc.levelIndex = _levelIndex;

    _skillPanel = Create_Child<UI_PlayerSkill>(Protocol::OBJECT_TYPE_UI_PLAYER_SKILL, EUILayer::HUD, &skillDesc);
    CHECK_NULL(_skillPanel, E_FAIL);

    _skillPanel->Get_Transform()->Set_LocalPosition(
        GAME->Get_WindowWidth() - 220.f,
        GAME->Get_WindowHeight() - 110.f,
        _zOrder);

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
