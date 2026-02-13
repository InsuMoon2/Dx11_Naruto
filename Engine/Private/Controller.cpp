#include "pch.h"
#include "Controller.h"

Controller::Controller(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : Component(device, context)
{
}

Controller::Controller(const Controller& rhs)
    : Component(rhs)
{
}

Controller::~Controller()
{
}

HRESULT Controller::Initialize_Prototype()
{
    Component::Initialize_Prototype();

    return S_OK;
}

HRESULT Controller::Initialize(void* arg)
{
    Component::Initialize(arg);

    return S_OK;
}

json Controller::To_Json() const
{
    json j = Component::To_Json();


    return j;
}

void Controller::From_Json(const json& data)
{
    Component::From_Json(data);


}

void Controller::Free()
{
    Component::Free();


}
