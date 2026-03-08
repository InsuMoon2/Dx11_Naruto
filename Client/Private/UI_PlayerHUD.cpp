#include "pch.h"
#include "UI_PlayerHUD.h"
#include "GameInstance.h"
#include "UI_PlayerStatus.h"

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

    UIObject::FUIDesc statDesc;
    statDesc.posX = 0.f;
    statDesc.posY = 0.f;
    statDesc.sizeX = 250.f;
    statDesc.sizeY = 80.f;
    statDesc.zOrder = _zOrder;
    statDesc.levelIndex = _levelIndex;

    _status = Create_Child<UI_PlayerStatus>(EUILayer::HUD, &statDesc);
    CHECK_NULL(_status, E_FAIL);

    _status->Get_Transform()->Set_LocalPosition(200.f, 750.f, _zOrder);

    return S_OK;
}

void UI_PlayerHUD::Update(float timeDelta)
{
    HUD::Update(timeDelta);
}

void UI_PlayerHUD::Bind_Player(Shared<Player> player)
{
    if (_status)
        _status->Bind_Player(player);
}

Shared<UI_PlayerHUD> UI_PlayerHUD::Create(ComPtr<Device> device, ComPtr<DeviceContext> context, void* arg)
{
    auto instance = make_shared<UI_PlayerHUD>(device, context);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : PlayerHUD");
        return nullptr;
    }

    return instance;
}

void UI_PlayerHUD::Free()
{
    HUD::Free();
}
