#include "pch.h"
#include "Bounding.h"

Bounding::Bounding(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device), _context(context)
{
}

HRESULT Bounding::Initialize(const FBoundingDesc& desc)
{

    return S_OK;
}

void Bounding::Free()
{
    Base::Free();
}
