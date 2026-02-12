#include "pch.h"
#include "Controller.h"

Controller::Controller(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

Controller::Controller(const Controller& rhs)
    : GameObject(rhs)
{
}

Controller::~Controller()
{
}

HRESULT Controller::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT Controller::Initialize(void* arg)
{
    return GameObject::Initialize(arg);
}

void Controller::Priority_Update(float timeDelta)
{
    GameObject::Priority_Update(timeDelta);
}

void Controller::Update(float timeDelta)
{
    GameObject::Update(timeDelta);
}

void Controller::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);
}

HRESULT Controller::Render()
{
    GameObject::Render();

    return S_OK;
}

void Controller::Free()
{
    GameObject::Free();
}
