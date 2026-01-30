#include "pch.h"
#include "Background.h"
#include "GameInstance.h"

Background::Background(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject { device, context }
{
}

Background::Background(const Background& rhs)
    : GameObject { rhs }
{
}

Background::~Background()
{
}

HRESULT Background::Initialize_Prototype()
{
    

    return S_OK;
}

HRESULT Background::Initialize(any arg)
{
    GameObject::Initialize(arg);

    return S_OK;
}

void Background::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void Background::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void Background::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    GAME->Add_RenderGroup(ERenderGroup::UI, GetSharedPtr());
}

shared_ptr<GameObject> Background::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Background>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : Background");

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> Background::Clone(any arg)
{
    auto instance = make_shared<Background>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        MSG_BOX("Failed to Created : Background");

        return nullptr;
    }

    return instance;
}

void Background::Free()
{
    GameObject::Free();
}
