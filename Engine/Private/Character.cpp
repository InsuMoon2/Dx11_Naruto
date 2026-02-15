#include "pch.h"
#include "Character.h"

#include "Controller.h"

Character::Character(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

Character::Character(const Character& rhs)
    : GameObject(rhs)
{
}

Character::~Character()
{
}

HRESULT Character::Initialize_Prototype()
{
    GameObject::Initialize_Prototype();

    return S_OK;
}

HRESULT Character::Initialize(void* arg)
{
    GameObject::Initialize(arg);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void Character::BeginPlay()
{
    GameObject::BeginPlay();


}

void Character::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void Character::Update(float timeDelta)
{
    GameObject::Update(timeDelta);


}

void Character::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT Character::Render()
{
    GameObject::Render();

    return S_OK;
}

HRESULT Character::Ready_Components()
{
    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    return S_OK;
}

void Character::Free()
{
    GameObject::Free();
}
