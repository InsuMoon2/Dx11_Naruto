#include "pch.h"
#include "HUD.h"

HUD::HUD(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : UIObject(device, context)
{
}

HUD::HUD(const HUD& rhs)
    : UIObject(rhs)
{
}

HRESULT HUD::Initialize_Prototype()
{
    CHECK_FAILED(UIObject::Initialize_Prototype(), E_FAIL);

    return S_OK;
}

HRESULT HUD::Initialize(void* arg)
{
    CHECK_FAILED(UIObject::Initialize(arg), E_FAIL);

    return S_OK;
}

void HUD::Update(float timeDelta)
{
    UIObject::Update(timeDelta);
}

void HUD::Late_Update(float timeDelta)
{
    UIObject::Late_Update(timeDelta);
}

HRESULT HUD::Render()
{
    return UIObject::Render();
}

void HUD::Set_Visibility(bool active)
{
    UIObject::Set_Visibility(active);

    for (auto& child : _children)
    {
        if (child)
            child->Set_Visibility(active);
    }
}

void HUD::Register_Child(Shared<UIObject> child)
{
    if (!child)
        return;

    _children.push_back(child);
}

void HUD::Free()
{
    _children.clear();

    UIObject::Free();
}



