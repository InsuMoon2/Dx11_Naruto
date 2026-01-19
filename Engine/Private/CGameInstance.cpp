#include "pch.h"
#include "CGameInstance.h"
#include "CGraphic_Device.h"
#include "CLevel_Manager.h"
#include "CTimer_Manager.h"

IMPLEMENT_SINGLETON(CGameInstance)

CGameInstance::CGameInstance()
{
    
}

CGameInstance::~CGameInstance()
{
    
}

HRESULT CGameInstance::Initialize_Engine(const ENGINE_DESC& desc, ComPtr<Device>& deviceOut,
    ComPtr<DeviceContext>& contextOut)
{
    /* Graphic Device 초기화 */
    _graphicDevice = CGraphic_Device::Create(
        desc.hWnd, desc.winMode, desc.viewportWidth, desc.viewportHeight,
        deviceOut, contextOut);

    NULL_CHECK_RETURN(_graphicDevice, E_FAIL);

    _timerManager = CTimer_Manager::Create();
    NULL_CHECK_RETURN(_timerManager, E_FAIL);

    _levelManager = CLevel_Manager::Create();
    NULL_CHECK_RETURN(_levelManager, E_FAIL);
    

    return S_OK;
}

void CGameInstance::Update_Engine(float timeDelta)
{
    _levelManager->Update(timeDelta);
}

void CGameInstance::LateUpdate_Engine(float timeDelta)
{
    _levelManager->LateUpdate(timeDelta);
}

HRESULT CGameInstance::Draw()
{
    _levelManager->Render();

    return S_OK;
}

void CGameInstance::Clear_Resources(uint32 levelIndex)
{
}

HRESULT CGameInstance::Clear_Buffers(const color& clearColor)
{
    if (FAILED(_graphicDevice->Clear_BackBufferView(clearColor)))
        return E_FAIL;

    if (FAILED(_graphicDevice->Clear_DepthStencil_View()))
        return E_FAIL;

    return S_OK;
}

HRESULT CGameInstance::Present()
{
    if (_graphicDevice == nullptr)
        return E_FAIL;

    return _graphicDevice->Present();
}

HRESULT CGameInstance::Add_Timer(const wstring& timerTag)
{
    if (_timerManager == nullptr)
        return E_FAIL;

    return _timerManager->Add_Timer(timerTag);
}

float CGameInstance::Compute_TimeDelta(const wstring& timerTag)
{
    if (_timerManager == nullptr)
        return 0.f;

    return _timerManager->Compute_TimeDelta(timerTag);
}

HRESULT CGameInstance::Change_Level(uint32 levelIndex, shared_ptr<CLevel> level)
{
    return _levelManager->Change_Level(levelIndex, level);
}

void CGameInstance::Free()
{
    CBase::Free();

    _timerManager.reset();
    _graphicDevice.reset();
}
