#include "pch.h"
#include "PlayerStart.h"

PlayerStart::PlayerStart(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
    Set_ObjectType(Protocol::OBJECT_TYPE_PLAYER_START);
}

PlayerStart::PlayerStart(const PlayerStart& rhs)
    : GameObject(rhs)
    , _spawnIndex(rhs._spawnIndex)
{
}

PlayerStart::~PlayerStart()
{
}

HRESULT PlayerStart::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT PlayerStart::Initialize(void* arg)
{
    if (arg == nullptr)
        return S_OK;

    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);

    FPlayerStartDesc* desc = static_cast<FPlayerStartDesc*>(arg);
    _spawnIndex = desc->spawnIndex;

    return S_OK;
}

void PlayerStart::BeginPlay()
{
    GameObject::BeginPlay();

}

void PlayerStart::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);

}

void PlayerStart::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

}

void PlayerStart::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

}

HRESULT PlayerStart::Render()
{
    //GameObject::Render();

    return S_OK;
}

Shared<PlayerStart> PlayerStart::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<PlayerStart>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : PlayerStart");
        instance->Free();

        return nullptr;
    }

    return instance;
}

Shared<GameObject> PlayerStart::Clone(void* arg)
{
    auto clone = make_shared<PlayerStart>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : PlayerStart");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void PlayerStart::Free()
{
    GameObject::Free();
}
