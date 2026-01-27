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

HRESULT Component::Initialize()
{

    return S_OK;
}

HRESULT Component::Initialize_Prototype()
{

    return S_OK;
}

void Component::Free()
{
    Base::Free();

}
