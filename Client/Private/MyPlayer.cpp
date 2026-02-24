#include "pch.h"
#include "MyPlayer.h"

MyPlayer::MyPlayer(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Player(device, context)
{
}

MyPlayer::MyPlayer(const MyPlayer& rhs)
    : Player(rhs)
{
}

HRESULT MyPlayer::Initialize_Prototype()
{
    return Player::Initialize_Prototype();
}

HRESULT MyPlayer::Initialize(void* arg)
{
    return Player::Initialize(arg);
}

void MyPlayer::Update(float timeDelta)
{
    Player::Update(timeDelta);
}

void MyPlayer::Late_Update(float timeDelta)
{
    Player::Late_Update(timeDelta);
}

void MyPlayer::Send_MovePacket()
{
}

HRESULT MyPlayer::Ready_Components()
{
}

shared_ptr<GameObject> MyPlayer::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<MyPlayer>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : MyPlayer");
        instance->Free();

        return nullptr;
    }

    return instance;
}

shared_ptr<GameObject> MyPlayer::Clone(void* arg)
{
    auto clone = make_shared<MyPlayer>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : MyPlayer");
        clone->Free();

        return nullptr;
    }

    return clone;
}

void MyPlayer::Free()
{
    Player::Free();

}
