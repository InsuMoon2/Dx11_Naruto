#include "pch.h"
#include "CLevel.h"
#include "CGameInstance.h"

CLevel::CLevel(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device)
    , _context(context)
{

}

CLevel::~CLevel()
{
    
}


HRESULT CLevel::Initialize()
{


    return S_OK;
}

void CLevel::Update(float timeDelta)
{

}

void CLevel::LateUpdate(float timeDelta)
{

}

HRESULT CLevel::Render()
{

    return S_OK;
}

void CLevel::Free()
{
    CBase::Free();

}
