#include "pch.h"
#include "Panel.h"

Panel::Panel(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : HUD(device, context)
{
}

Panel::Panel(const Panel& rhs)
    : HUD(rhs)
{
}

HRESULT Panel::Initialize_Prototype()
{
    return HUD::Initialize_Prototype();
}

HRESULT Panel::Initialize(void* arg)
{
    return HUD::Initialize(arg);
}

void Panel::Free()
{
    HUD::Free();
}
