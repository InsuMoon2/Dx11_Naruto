#include "pch.h"
#include "Character.h"
#include "Controller.h"

Character::Character(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : ContainerObject(device, context)
{
}

Character::Character(const Character& rhs)
    : ContainerObject(rhs)
{
}

Character::~Character()
{
}

HRESULT Character::Initialize_Prototype()
{
    ContainerObject::Initialize_Prototype();

    return S_OK;
}

HRESULT Character::Initialize(void* arg)
{
    ContainerObject::Initialize(arg);

    CHECK_FAILED(Ready_Components(), E_FAIL);

    return S_OK;
}

void Character::BeginPlay()
{
    ContainerObject::BeginPlay();


}

void Character::Priority_Update(float timeDelta)
{
    ContainerObject::Priority_Update(timeDelta);
}

void Character::Update(float timeDelta)
{
    ContainerObject::Update(timeDelta);


}

void Character::Late_Update(float timeDelta)
{
    ContainerObject::Late_Update(timeDelta);
}

HRESULT Character::Render()
{
    ContainerObject::Render();

    return S_OK;
}

HRESULT Character::Bind_Lights()
{

    return S_OK;
}

void Character::TakeDamage(const FDamageEvent& damageEvent)
{
    
}

HRESULT Character::Ready_Components()
{
    _transformCom->Set_LocalPosition(0.f, 0.f, -5.f);

    return S_OK;
}

void Character::Free()
{
    ContainerObject::Free();
}
