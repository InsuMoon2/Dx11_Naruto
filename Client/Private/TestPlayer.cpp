#include "pch.h"
#include "TestPlayer.h"

TestPlayer::TestPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

TestPlayer::TestPlayer(const TestPlayer& rhs)
    : GameObject(rhs)
{

}

HRESULT TestPlayer::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT TestPlayer::Initialize(any arg)
{
    return GameObject::Initialize(arg);
}

void TestPlayer::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void TestPlayer::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void TestPlayer::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

shared_ptr<GameObject> TestPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<TestPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> TestPlayer::Clone(any arg)
{
    auto instance = make_shared<TestPlayer>(*this);

    if (FAILED(instance->Initialize(arg)))
    {
        return nullptr;
    }

    return instance;
}
