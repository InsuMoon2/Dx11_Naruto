#include "pch.h"
#include "Component.h"

Component::Component(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

Component::Component(const Component& rhs)
    : _device(rhs._device), _context(rhs._context)
{
}

Component::~Component()
{
}

HRESULT Component::Initialize(void* arg)
{

    return S_OK;
}

json Component::To_Json() const
{
    json j;

    auto id = static_cast<Protocol::ComponentID>(Get_ComponentID());
    j["type"] = string(magic_enum::enum_name(id));

    return j;
}

void Component::From_Json(const json& data)
{

}

HRESULT Component::Initialize_Prototype()
{

    return S_OK;
}

void Component::Free()
{
    Base::Free();

}
