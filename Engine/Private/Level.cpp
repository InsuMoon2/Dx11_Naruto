#include "pch.h"
#include "Level.h"
#include "GameInstance.h"

Level::Level(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : _device(device)
    , _context(context)
{

}

Level::~Level()
{
    
}


HRESULT Level::Initialize()
{


    return S_OK;
}

void Level::Update(float timeDelta)
{

}

void Level::Late_Update(float timeDelta)
{

}

HRESULT Level::Render()
{

    return S_OK;
}

void Level::Free()
{
    Base::Free();

}
