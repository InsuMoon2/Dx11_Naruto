#include "pch.h"
#include "GameInstance.h"
#include "Graphic_Device.h"

#include "Level_Manager.h"
#include "Prototype_Manager.h"
#include "Timer_Manager.h"
#include "Prototype_Manager.h"
#include "Object_Manager.h"
#include "Input_Manager.h"

#include "GameObject.h"
#include "Component.h"
#include "Renderer.h"

IMPLEMENT_SINGLETON(GameInstance)

GameInstance::GameInstance()
{
    
}

GameInstance::~GameInstance()
{
    
}

HRESULT GameInstance::Initialize_Engine(const ENGINE_DESC& desc, ComPtr<Device>& deviceOut,
    ComPtr<DeviceContext>& contextOut)
{
    /* Graphic Device 초기화 */
    _graphicDevice = Graphic_Device::Create(
        desc.hWnd, desc.winMode, desc.viewportWidth, desc.viewportHeight,
        deviceOut, contextOut);

    NULL_CHECK_RETURN(_graphicDevice, E_FAIL);

    INPUT->Init(desc.hWnd);

    _timerManager = Timer_Manager::Create();
    NULL_CHECK_RETURN(_timerManager, E_FAIL);

    _levelManager = Level_Manager::Create();
    NULL_CHECK_RETURN(_levelManager, E_FAIL);

    _protoManager = Prototype_Manager::Create(desc.numLevels);
    NULL_CHECK_RETURN(_protoManager, E_FAIL);

    _objectManager = Object_Manager::Create(desc.numLevels);
    NULL_CHECK_RETURN(_objectManager, E_FAIL);
    
    _renderer = Renderer::Create(Get_Device(), Get_Context());
    NULL_CHECK_RETURN(_renderer, E_FAIL);

    return S_OK;
}

void GameInstance::Priority_Update_Engine(float timeDelta)
{
    _objectManager->Priority_Update(timeDelta);

}

void GameInstance::Update_Engine(float timeDelta)
{
    INPUT->Update(timeDelta);
    _levelManager->Update(timeDelta);
    _objectManager->Update(timeDelta);


}

void GameInstance::Late_Update_Engine(float timeDelta)
{
    _levelManager->Late_Update(timeDelta);
    _objectManager->Late_Update(timeDelta);
}

HRESULT GameInstance::Draw()
{
    _levelManager->Render();

    return S_OK;
}

void GameInstance::Clear_Resources(uint32 levelIndex)
{
}

ComPtr<Device> GameInstance::Get_Device()
{
    return _graphicDevice->Get_Device();
}

ComPtr<DeviceContext> GameInstance::Get_Context()
{
    return _graphicDevice->Get_Context();
}

uint32 GameInstance::Get_ViewportWidth()
{
    return _graphicDevice->GetWidth();
}

uint32 GameInstance::Get_ViewportHeight()
{
    return _graphicDevice->GetHeight();
}

void GameInstance::BindBackBuffer()
{
    _graphicDevice->BindBackBuffer();
}

HRESULT GameInstance::Resize_BackBuffer(uint32 width, uint32 height)
{
    return _graphicDevice->Resize(width, height);
}

HRESULT GameInstance::Clear_Buffers(const Color& clearColor)
{
    if (FAILED(_graphicDevice->Clear_BackBufferView(clearColor)))
        return E_FAIL;

    if (FAILED(_graphicDevice->Clear_DepthStencil_View()))
        return E_FAIL;

    return S_OK;
}

HRESULT GameInstance::Present()
{
    if (_graphicDevice == nullptr)
        return E_FAIL;

    return _graphicDevice->Present();
}

HRESULT GameInstance::Add_Timer(const wstring& timerTag)
{
    if (_timerManager == nullptr)
        return E_FAIL;

    return _timerManager->Add_Timer(timerTag);
}

float GameInstance::Compute_TimeDelta(const wstring& timerTag)
{
    if (_timerManager == nullptr)
        return 0.f;

    return _timerManager->Compute_TimeDelta(timerTag);
}

HRESULT GameInstance::Change_Level(uint32 levelIndex, shared_ptr<Level> level)
{
    return _levelManager->Change_Level(levelIndex, level);
}

HRESULT GameInstance::Add_GameObject_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<GameObject> gameObject)
{
    return _protoManager->Add_GameObject_Prototype(levelIndex, prototypeTag, gameObject);
}

shared_ptr<GameObject> GameInstance::Clone_GameObject(uint32 levelIndex, const wstring& prototypeTag, any arg)
{
    return _protoManager->Clone_GameObject(levelIndex, prototypeTag, arg);
}

HRESULT GameInstance::Add_Component_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<Component> component)
{
    return _protoManager->Add_Component_Prototype(levelIndex, prototypeTag, component);
}

shared_ptr<Component> GameInstance::Clone_Component(uint32 levelIndex, const wstring& prototypeTag, any arg)
{
    return _protoManager->Clone_Component(levelIndex, prototypeTag, arg);
}

HRESULT GameInstance::Add_GameObject(uint32 protoIndex, const wstring& protoTag, uint32 layerIndex,
    const wstring& layerTag, any arg)
{
    return _objectManager->Add_GameObject(protoIndex, protoTag, layerIndex, layerTag, arg);
}

void GameInstance::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    return _renderer->Add_RenderGroup(renderType, gameObject);
}

void GameInstance::Free()
{
    Base::Free();

    _objectManager.reset(); 
    _levelManager.reset();  
    _protoManager.reset();  
    _timerManager.reset();  
    _graphicDevice.reset(); 
}
