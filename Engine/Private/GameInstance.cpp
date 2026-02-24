#include "pch.h"
#include "GameInstance.h"
#include "Graphic_Device.h"

#include "Level_Manager.h"
#include "Prototype_Manager.h"
#include "Timer_Manager.h"
#include "Prototype_Manager.h"
#include "Object_Manager.h"
#include "Input_Manager.h"
#include "Event_Manager.h"

#include "GameObject.h"
#include "Component.h"
#include "Prefab_Manager.h"
#include "Renderer.h"

#include "PipeLine.h"

#include "BTNode_Factory.h"
#include "Component_Factory.h"

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

    CHECK_NULL(_graphicDevice, E_FAIL);

    INPUT->Init(desc.hWnd);

    _timerManager = Timer_Manager::Create();
    CHECK_NULL(_timerManager, E_FAIL);

    _levelManager = Level_Manager::Create();
    CHECK_NULL(_levelManager, E_FAIL);

    _protoManager = Prototype_Manager::Create(desc.numLevels);
    CHECK_NULL(_protoManager, E_FAIL);

    _objectManager = Object_Manager::Create(desc.numLevels);
    CHECK_NULL(_objectManager, E_FAIL);
    
    _renderer = Renderer::Create(Get_Device(), Get_Context());
    CHECK_NULL(_renderer, E_FAIL);

    _prefabManager = Prefab_Manager::Create(Get_Device(), Get_Context());
    CHECK_NULL(_prefabManager, E_FAIL);

    _pipeLine = PipeLine::Create();
    CHECK_NULL(_pipeLine, E_FAIL);

    _componentFactory = Component_Factory::Create();
    CHECK_NULL(_componentFactory, E_FAIL);

    _btNodeFactory = BTNode_Factory::Create();
    CHECK_NULL(_btNodeFactory, E_FAIL);
    //_btNodeFactory->Register_EngineNodes();

    return S_OK;
}

void GameInstance::Priority_Update_Engine(float timeDelta)
{
    _objectManager->Priority_Update(timeDelta);

    _pipeLine->Update();
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

    EVENT->ProcessEvents();
}

HRESULT GameInstance::Draw()
{
    _renderer->Draw();
    _levelManager->Render();

    return S_OK;
}

void GameInstance::Clear_Resources(uint32 levelIndex)
{
    _objectManager->Clear_Layers(levelIndex);
    _protoManager->Clear_Prototype(levelIndex);
}

ComPtr<Device> GameInstance::Get_Device()
{
    return _graphicDevice->Get_Device();
}

ComPtr<DeviceContext> GameInstance::Get_Context()
{
    return _graphicDevice->Get_Context();
}

float GameInstance::Get_ViewportWidth()
{
    return _graphicDevice->Get_ViewportWidth();
}

float GameInstance::Get_ViewportHeight()
{
    return _graphicDevice->Get_ViewportHeight();
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

void GameInstance::Set_ImGuiContext(void* context)
{
    ImGui::SetCurrentContext(static_cast<ImGuiContext*>(context));
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

uint32 GameInstance::Current_Level()
{
    return _levelManager->Get_CurrentLevel();
}

HRESULT GameInstance::Add_GameObject_Prototype(uint32 levelIndex, uint32 objID, shared_ptr<GameObject> gameObject)
{
    return _protoManager->Add_GameObject_Prototype(levelIndex, objID, gameObject);
}

shared_ptr<GameObject> GameInstance::Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg)
{
    return _protoManager->Clone_GameObject(levelIndex, objID, arg);
}

HRESULT GameInstance::Add_Component_Prototype(uint32 levelIndex, uint32 componentID, shared_ptr<Component> component)
{
    return _protoManager->Add_Component_Prototype(levelIndex, componentID, component);
}

shared_ptr<Component> GameInstance::Clone_Component(uint32 levelIndex, uint32 componentID, void* arg)
{
    return _protoManager->Clone_Component(levelIndex, componentID, arg);
}

Shared<Component> GameInstance::Clone_Component(uint32 componentID, void* arg)
{
    return _protoManager->Clone_Component(componentID, arg);
}

vector<pair<uint32, wstring>> GameInstance::Get_RegisteredGameObjects()
{
    return _protoManager->Get_RegisteredGameObjects();
}

HRESULT GameInstance::Add_GameObject(uint32 protoIndex, uint32 objID, uint32 layerIndex, const wstring& layerTag, void* arg)
{
    return _objectManager->Add_GameObject(protoIndex, objID, layerIndex, layerTag, arg);
}

HRESULT GameInstance::Add_GameObject(uint32 levelIndex, uint32 objID, const wstring& layerTag, void* arg)
{
    return _objectManager->Add_GameObject(levelIndex, objID, levelIndex, layerTag, arg);
}

HRESULT GameInstance::Add_GameObject(uint32 levelIndex, const wstring& layerTag, Shared<GameObject> gameObject)
{
    return _objectManager->Add_GameObject(levelIndex, layerTag, gameObject);
}

vector<shared_ptr<GameObject>> GameInstance::Get_GameObjects(uint32 levelIndex)
{
    return _objectManager->Get_GameObjects(levelIndex);
}

const umap<wstring, Shared<Layer>> GameInstance::Get_Layers(uint32 levelIndex)
{
    return _objectManager->Get_Layers(levelIndex);
}

void GameInstance::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    return _renderer->Add_RenderGroup(renderType, gameObject);
}

int32 GameInstance::Get_DrawCallCount()
{
    return _renderer->Get_DrawCallCount();
}

Shared<GameObject> GameInstance::Instantiate_Prefab(const string& prefabName, const json& overrides)
{
    return _prefabManager->Instantiate_Prefab(prefabName, overrides);
}

HRESULT GameInstance::Save_Prefab(const string& prefabPath, shared_ptr<GameObject> gameObject)
{
    return _prefabManager->Save_Prefab(prefabPath, gameObject);
}

HRESULT GameInstance::Load_Prefab(const string& prefabPath)
{
    return _prefabManager->Load_Prefab(prefabPath);
}

const Matrix* GameInstance::Get_Transform(ETransformState state) const
{
    return _pipeLine->Get_Transform(state);
}

const Vec4* GameInstance::Get_CamPosition() const
{
    return _pipeLine->Get_CamPosition();
}

void GameInstance::Set_Transform(ETransformState state, const Matrix& matrix)
{
    return _pipeLine->Set_Transform(state, matrix);
}

HRESULT GameInstance::Bind_TransformMatrix(ETransformState state, Shared<Shader> shader, const char* constantName)
{
    return _pipeLine->Bind_TransformMatrix(state, shader, constantName);
}

HRESULT GameInstance::Bind_TransformMatrix_Invsere(ETransformState state, Shared<Shader> shader,
    const char* constantName)
{
    return _pipeLine->Bind_TransformMatrix_Inverse(state, shader, constantName);
}

void GameInstance::Register_ComponentFactory(uint32 typeId, Component_Factory::Creator creator,
                                             const wstring& className)
{
    _componentFactory->Register(typeId, creator, className);
}

void GameInstance::Register_ComponentFactory_Prototype(uint32 typeId, uint32 levelIndex)
{
    _componentFactory->Register_Prototype(typeId, levelIndex, Get_Device(), Get_Context());
}

Shared<Component> GameInstance::Instantiate_FromFactory(uint32 typeId)
{
    return _componentFactory->Instantiate(typeId, Get_Device(), Get_Context());
}

vector<pair<uint32, wstring>> GameInstance::Get_RegisteredComponents()
{
    return _componentFactory->Get_RegisteredComponents();
}

void GameInstance::Register_BTNode(const string& category, const string& typeName, BTNode_Factory::Creator creator)
{
    _btNodeFactory->Register(category, typeName, creator);
}

Shared<BTNode> GameInstance::Instantiate_BTNode(const string& typeName)
{
    return _btNodeFactory->Instantiate(typeName);
}

const umap<string, BTNode_Factory::NodeInfo>& GameInstance::Get_RegisteredBTNodes() const
{
    return _btNodeFactory->Get_RegisteredNodes();
}

void GameInstance::Free()
{
    Base::Free();

    _componentFactory.reset();
    _btNodeFactory.reset();

    _prefabManager.reset();
    _objectManager.reset(); 
    _levelManager.reset();  
    _protoManager.reset();  
    _timerManager.reset();  
    _graphicDevice.reset();

    _renderer.reset();
    _pipeLine.reset();
}
