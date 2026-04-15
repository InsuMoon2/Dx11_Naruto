#include "pch.h"
#include "GameInstance.h"

#include "Asset_Manager.h"
#include "Graphic_Device.h"

#include "Level_Manager.h"
#include "Prototype_Manager.h"
#include "Timer_Manager.h"
#include "Prototype_Manager.h"
#include "Object_Manager.h"
#include "Input_Manager.h"
#include "Event_Manager.h"
#include "Camera_Manager.h"

#include "GameObject.h"
#include "Component.h"
#include "Prefab_Manager.h"
#include "Renderer.h"

#include "PipeLine.h"

#include "BTNode_Factory.h"
#include "Camera.h"
#include "Component_Factory.h"
#include "Light_Manager.h"
#include "UI_Manager.h"
#include "UIObject.h"

#include "Animation_Manager.h"
#include "GameObject_Factory.h"
#include "Text_Renderer.h"


#pragma push_macro("new")
#undef new
#include "Camera.h"
#include "Collision_Manager.h"
#include "Debug_Manager.h"
#include "imgui.h"
#include "Sound_Manager.h"
#pragma pop_macro("new")

IMPLEMENT_SINGLETON(GameInstance)

// 에디터/런타임 시작 직후 특정 모델 이름이 어떤 경로/GUID로 등록됐는지 빠르게 확인하기 위한 진단 로그를 남긴다.
static void Log_SkeletalModelRegistrationState(GameInstance* gameInstance, const string& targetStemName)
{
    if (!gameInstance || targetStemName.empty())
        return;

    auto assets = gameInstance->Get_AssetByType("model");
    if (assets.empty())
        assets = gameInstance->Get_AssetByType("Model");

    vector<const FAssetMeta*> matchedAssets;
    for (const FAssetMeta* meta : assets)
    {
        if (!meta)
            continue;

        if (meta->modelType != "SkeletalMesh")
            continue;

        const fs::path assetPath(meta->fullPath);
        if (Utils::ToLowerCopy(assetPath.stem().string()) != Utils::ToLowerCopy(targetStemName))
            continue;

        matchedAssets.push_back(meta);
    }

    if (matchedAssets.empty())
    {
        LOG_INFO("Skeletal model registration check - '{}' not found", targetStemName);
        return;
    }

    LOG_INFO("Skeletal model registration check - '{}' count = {}", targetStemName, matchedAssets.size());

    for (const FAssetMeta* meta : matchedAssets)
    {
        if (!meta)
            continue;

        LOG_INFO("  guid={} relativePath={} fullPath={}",
            meta->guid,
            Utils::ToString(meta->relativePath),
            Utils::ToString(meta->fullPath));
    }
}

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

    _uiViewportWidth = static_cast<float>(desc.viewportWidth);
    _uiViewportHeight = static_cast<float>(desc.viewportHeight);

    // 실제 창 크기와 별개로 UI 기준 캔버스를 저장
    _uiReferenceWidth = (desc.uiReferenceWidth > 0)
        ? static_cast<float>(desc.uiReferenceWidth)
        : _uiViewportWidth;

    _uiReferenceHeight = (desc.uiReferenceHeight > 0)
        ? static_cast<float>(desc.uiReferenceHeight)
        : _uiViewportHeight;

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

    _cameraManager = Camera_Manager::Create();
    CHECK_NULL(_cameraManager, E_FAIL);

    _lightManager = Light_Manager::Create();
    CHECK_NULL(_lightManager, E_FAIL);

    _assetManager = Asset_Manager::Create(TEXT("../../Client/Bin/Resources"));
    CHECK_NULL(_assetManager, E_FAIL);
    Log_SkeletalModelRegistrationState(this, "WhiteZetsu");

    _uiManager = UI_Manager::Create();
    CHECK_NULL(_uiManager, E_FAIL);

    _textRenderer = Text_Renderer::Create(Get_Device(), _graphicDevice->Get_SwapChain());
    CHECK_NULL(_textRenderer, E_FAIL);

    // 노티파이 경로 세팅
    _animationManager = Animation_Manager::Create(TEXT("../../Client/Bin/Resources/Models"));
    CHECK_NULL(_animationManager, E_FAIL);

    _gameObjectFactory = GameObject_Factory::Create();
    CHECK_NULL(_gameObjectFactory, E_FAIL);

    _soundManager = Sound_Manager::Create(TEXT("../../Client/Bin/Resources/Sounds"));
    CHECK_NULL(_soundManager, E_FAIL);

    _collisionManager = Collision_Manager::Create();
    CHECK_NULL(_collisionManager, E_FAIL);

    _debugManager = Debug_Manager::Create(Get_Device(), Get_Context());
    CHECK_NULL(_debugManager, E_FAIL);

    return S_OK;
}

void GameInstance::Priority_Update_Engine(float timeDelta)
{
    if (_debugManager)
        _debugManager->Tick(timeDelta);

    _objectManager->Priority_Update(timeDelta, _levelManager->Get_CurrentLevel());
    _cameraManager->Update(timeDelta);

    _pipeLine->Update();

    _uiManager->Priority_Update(timeDelta);
 }

void GameInstance::Update_Engine(float timeDelta)
{
    //INPUT->Update(timeDelta);
    _levelManager->Update(timeDelta);

    // 현재 레벨만 업데이트
    _objectManager->Update(timeDelta, _levelManager->Get_CurrentLevel());
    _uiManager->Update(timeDelta);

}

void GameInstance::Late_Update_Engine(float timeDelta)
{
    _collisionManager->Clear_Colliders();

    _levelManager->Late_Update(timeDelta);
    _objectManager->Late_Update(timeDelta, _levelManager->Get_CurrentLevel());
    _uiManager->Late_Update(timeDelta);

    _collisionManager->Update();

    EVENT->ProcessEvents();
}

void GameInstance::Update_CameraOnly(float timeDelta)
{
    if (_debugManager)
        _debugManager->Tick(timeDelta);

    auto activeCamera = _cameraManager->Get_ActiveCamera();
    if (activeCamera)
    {
        activeCamera->Priority_Update(timeDelta);
    }

    _pipeLine->Update();
}

HRESULT GameInstance::Draw(bool renderDebugPrimitives, bool renderColliders)
{
    _renderer->Draw(renderDebugPrimitives, renderColliders);
    _levelManager->Render();

    return S_OK;
}

void GameInstance::Clear_Resources(uint32 levelIndex)
{
    _objectManager->Clear_Layers(levelIndex);
    _protoManager->Clear_Prototype(levelIndex);
    _uiManager->Clear_UI_ByLevel(levelIndex);

    if (_debugManager)
        _debugManager->Clear();

    _cameraManager->Clear_InvalidCameras();
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

float GameInstance::Get_WindowWidth()
{
    return _graphicDevice->Get_WindowWidth();
}

float GameInstance::Get_WindowHeight()
{
    return _graphicDevice->Get_WindowHeight();
}

void GameInstance::BindBackBuffer()
{
    _graphicDevice->BindBackBuffer();
}

HRESULT GameInstance::Resize_BackBuffer(uint32 width, uint32 height)
{
    if (_textRenderer)
        _textRenderer->On_BeforeResize();

    CHECK_FAILED(_graphicDevice->Resize(width, height), E_FAIL);

    if (_textRenderer)
        CHECK_FAILED(_textRenderer->On_AfterResize(), E_FAIL);

    return S_OK;
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

float GameInstance::Get_UIViewportWidth() const
{
    return (_uiViewportWidth > 0.f) ? _uiViewportWidth : _graphicDevice->Get_ViewportWidth();
}

float GameInstance::Get_UIViewportHeight() const
{
    return (_uiViewportHeight > 0.f) ? _uiViewportHeight : _graphicDevice->Get_ViewportHeight();
}

void GameInstance::Set_UIViewportSize(float width, float height)
{
    _uiViewportWidth = width;
    _uiViewportHeight = height;
}

float GameInstance::Get_UIReferenceWidth() const
{
    return (_uiReferenceWidth > 0.f) ? _uiReferenceWidth : Get_UIViewportWidth();
}

float GameInstance::Get_UIReferenceHeight() const
{
    return (_uiReferenceHeight > 0.f) ? _uiReferenceHeight : Get_UIViewportHeight();
}

void GameInstance::Set_UIReferenceSize(float width, float height)
{
    _uiReferenceWidth = width;
    _uiReferenceHeight = height;
}

float GameInstance::Get_UIScale() const
{
    const float refW = Get_UIReferenceWidth();
    const float refH = Get_UIReferenceHeight();
    const float viewW = Get_UIViewportWidth();
    const float viewH = Get_UIViewportHeight();

    if (refW <= 0.f || refH <= 0.f)
        return 1.f;

    // 비율 유지용 단일 스케일
    return min(viewW / refW, viewH / refH);
}

Vec2 GameInstance::Get_UIViewportOffset() const
{
    const float scale = Get_UIScale();
    const float contentW = Get_UIReferenceWidth() * scale;
    const float contentH = Get_UIReferenceHeight() * scale;

    const float offsetX = (Get_UIViewportWidth() - contentW) * 0.5f;
    const float offsetY = (Get_UIViewportHeight() - contentH) * 0.5f;

    return Vec2(offsetX, offsetY);
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

Shared<Level> GameInstance::Get_Current_Level()
{
    return _levelManager->Get_CurrentLevelType();
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

Shared<Component> GameInstance::Find_Component_Prototype(uint32 levelIndex, uint32 componentID)
{
    return _protoManager->Find_Component_Prototype(levelIndex, componentID);
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

Shared<GameObject> GameInstance::Clone_And_Add_GameObject(uint32 protoIndex, uint32 objID, uint32 levelIndex, const wstring& layerTag, void* arg)
{
    return _objectManager->Clone_And_Add_GameObject(protoIndex, objID, levelIndex, layerTag, arg);
}

vector<shared_ptr<GameObject>> GameInstance::Get_GameObjects(uint32 levelIndex)
{
    return _objectManager->Get_GameObjects(levelIndex);
}

const umap<wstring, Shared<Layer>> GameInstance::Get_Layers(uint32 levelIndex)
{
    return _objectManager->Get_Layers(levelIndex);
}

void GameInstance::Clear_Layers(uint32 levelIndex)
{
    return _objectManager->Clear_Layers(levelIndex);
}

void GameInstance::Delete_GameObject(uint32 levelIndex, Shared<GameObject> gameObject)
{
    return _objectManager->Delete_GameObject(levelIndex, gameObject);
}

void GameInstance::Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject)
{
    return _renderer->Add_RenderGroup(renderType, gameObject);
}

int32 GameInstance::Get_DrawCallCount()
{
    return _renderer->Get_DrawCallCount();
}

void GameInstance::Backup_RenderGroup()
{
    _renderer->Backup_RenderGroup();
}

void GameInstance::Restore_RenderGroup()
{
    _renderer->Restore_RenderGroup();
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

void GameInstance::Reapply_Prefabs_InCurrentLevel()
{
    _prefabManager->Reapply_Prefabs_InLevel(Current_Level());
}

const Matrix* GameInstance::Get_Transform(ETransformState state) const
{
    return _pipeLine->Get_Transform(state);
}

const Matrix* GameInstance::Get_TransformInverse(ETransformState state) const
{
    return _pipeLine->Get_TransformInverse(state);
}

const Vec4* GameInstance::Get_CamPosition() const
{
    return _pipeLine->Get_CamPosition();
}

void GameInstance::Set_Transform(ETransformState state, const Matrix& matrix)
{
    return _pipeLine->Set_Transform(state, matrix);
}

HRESULT GameInstance::Bind_CamPosition(Shared<Shader> shader, const char* constantName)
{
    return _pipeLine->Bind_CamPosition(shader, constantName);
}

HRESULT GameInstance::Bind_TransformMatrix(ETransformState state, Shared<Shader> shader, const char* constantName)
{
    return _pipeLine->Bind_TransformMatrix(state, shader, constantName);
}

HRESULT GameInstance::Bind_TransformMatrix_Inverse(ETransformState state, Shared<Shader> shader,
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

Shared<GameObject> GameInstance::Create_GameObjectFromFactory(Protocol::OBJECT_TYPE type)
{
    return _gameObjectFactory->Create_Object(Get_Device(), Get_Context(), type);
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
 
void GameInstance::Set_ActiveCamera(Shared<Camera> camera)
{
    _cameraManager->Set_ActiveCamera(camera);
}

Shared<Camera> GameInstance::Get_ActiveCamera()
{
    return _cameraManager->Get_ActiveCamera();
}

bool GameInstance::Is_ActiveCamera(Shared<Camera> camera)
{
    return _cameraManager->Is_ActiveCamera(camera);
}

void GameInstance::Toggle_Camera()
{
    _cameraManager->Toggle_Camera();
}

void GameInstance::Register_Camera(Shared<Camera> camera)
{
    _cameraManager->Register_Camera(camera);
}

const FLightDesc* GameInstance::Get_LightDesc(uint32 index)
{
    return _lightManager->Get_LightDesc(index);
}

HRESULT GameInstance::Add_Light(const FLightDesc& desc)
{
    return _lightManager->Add_Light(desc);
}

void GameInstance::Clear_Lights()
{
    return _lightManager->Clear_Lights();
}

string GameInstance::Find_AssetGUID(const wstring& filePath)
{
    return _assetManager->Find_GUID(filePath);
}

const FAssetMeta* GameInstance::Find_AssetByGUID(const string& guid)
{
    return _assetManager->Find_ByGUID(guid);
}

wstring GameInstance::Resolve_AssetPath(const string& guid)
{
    return _assetManager->Resolve_Path(guid);
}

string GameInstance::Register_Asset(const wstring& filePath, const string& type)
{
    return _assetManager->Register_Asset(filePath, type);
}

void GameInstance::Scan_Assets(const wstring& directory)
{
    return _assetManager->Scan_And_Register(directory);
}

vector<const FAssetMeta*> GameInstance::Get_AssetByType(const string& type)
{
    return _assetManager->Get_AssetByType(type);
}

void GameInstance::Update_AssetPath(const string& guid, const wstring& newFilePath)
{
    return _assetManager->Update_AssetPath(guid, newFilePath);
}

void GameInstance::Refresh_Cache()
{
    return _assetManager->Refresh_Cache();
}

uint32 GameInstance::Clear_DisallowedMeta(const wstring& directory)
{
    return _assetManager->Clear_DisallowedMeta(directory);
}

Shared<UIObject> GameInstance::Find_UI(const wstring& name)
{
    return _uiManager->Find_UI(name);
}

void GameInstance::Show_UI(const wstring& name)
{
    return _uiManager->Show_UI(name);
}

void GameInstance::Hide_UI(const wstring& name)
{
    return _uiManager->Hide_UI(name);
}

void GameInstance::Toggle_UI(const wstring& name)
{
    return _uiManager->Toggle_UI(name);
}

void GameInstance::Hide_All_Layer(EUILayer layer)
{
    return _uiManager->Hide_All_Layer(layer);
}

void GameInstance::Hide_All_UI()
{
    return _uiManager->Hide_All_UI();
}

bool GameInstance::Is_UIInputBlocked() const
{
    return _uiManager->Is_InputBlocked();
}

void GameInstance::Clear_UI()
{
    return _uiManager->Clear_All_UI();
}

void GameInstance::Clear_UI_ByLevel(uint32 levelIndex)
{
    return _uiManager->Clear_UI_ByLevel(levelIndex);
}

const list<Shared<UIObject>>& GameInstance::Get_UILayers(EUILayer layer) const
{
    return _uiManager->Get_UILayer(layer);
}

Shared<UIObject> GameInstance::Add_UI(uint32 objID, EUILayer layer, void* arg)
{
    return _uiManager->Add_UI(objID, layer, arg);
}

void GameInstance::Set_UIPrototypeLevel(uint32 levelIndex)
{
    _uiManager->Set_UIPrototypeLevel(levelIndex);
}

HRESULT GameInstance::Remove_UI(const wstring& name)
{
    return _uiManager->Remove_UI(name);
}

HRESULT GameInstance::Remove_UI(const Shared<UIObject>& uiObject)
{
    return _uiManager->Remove_UI(uiObject);
}

Shared<UIObject> GameInstance::Clone_UI(uint32 objID, void* arg)
{
    return _uiManager->Clone_UI(objID, arg);
}

HRESULT GameInstance::Register_UI(EUILayer layer, Shared<UIObject> uiObject)
{
    return _uiManager->Register_UI(layer, uiObject);
}

bool GameInstance::Play_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    return _uiManager->Play_UIAnimation(target, animationName);
}

bool GameInstance::Play_UIAnimation(const wstring& targetName, const string& animationName)
{
    return _uiManager->Play_UIAnimation(targetName, animationName);
}

bool GameInstance::Pause_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    return _uiManager->Pause_UIAnimation(target, animationName);
}

bool GameInstance::Pause_UIAnimation(const wstring& targetName, const string& animationName)
{
    return _uiManager->Pause_UIAnimation(targetName, animationName);
}

bool GameInstance::Stop_UIAnimation(Shared<UIObject> target, const string& animationName)
{
    return _uiManager->Stop_UIAnimation(target, animationName);
}

bool GameInstance::Stop_UIAnimation(const wstring& targetName, const string& animationName)
{
    return _uiManager->Stop_UIAnimation(targetName, animationName);
}

HRESULT GameInstance::Begin_UIText()
{
    return _textRenderer->Begin_UIText();
}

HRESULT GameInstance::Draw_Text(const wstring& text, const RECT& rect, const FTextStyle& style)
{
    return _textRenderer->Draw_Text(text, rect, style);
}

HRESULT GameInstance::End_UIText()
{
    CHECK_FAILED(_textRenderer->End_UIText(), E_FAIL);

    // D2D가 target을 사용한 뒤 다음 D3D 렌더 경로를 위해 백버퍼 상태를 복원한다.
    _graphicDevice->BindBackBuffer();

    return S_OK;
}

HRESULT GameInstance::Set_TextTarget_Texture(ComPtr<Texture2D> texture)
{
    return _textRenderer->Set_TargetTexture(texture);
}

HRESULT GameInstance::Reset_TextTarget_BackBuffer()
{
    return _textRenderer->Reset_TargetToSwapChain();
}

void GameInstance::Load_Animations_From_Directory(const wstring& directoryPath)
{
    return _animationManager->Load_Animations_From_Directory(directoryPath);
}

Shared<Animation> GameInstance::Get_Animation(const string& name) const
{
    return _animationManager->Get_Animation(name);
}

vector<Shared<Animation>> GameInstance::Get_Animations_By_Prefix(const string& prefix) const
{
    return _animationManager->Get_Animations_By_Prefix(prefix);
}

vector<Shared<Animation>> GameInstance::Get_All_Animations()
{
    return _animationManager->Get_All_Animations();
}

vector<Shared<Animation>> GameInstance::Get_Animations_InFolder(const string& folderPath)
{
    return _animationManager->Get_Animations_InFolder(folderPath);
}

bool GameInstance::Play_Sound(const wstring& soundFile, ESoundChannel channel, float volume)
{
    return _soundManager->Play_Once(soundFile, channel, volume);
}

bool GameInstance::Play_Sound_Pitched(const wstring& soundFile, ESoundChannel channel, float volume, float pitch)
{
    return _soundManager->Play_Once_Pitched(soundFile, channel, volume, pitch);
}

bool GameInstance::Play_BGM(const wstring& soundFile, float volume, bool stopPrevBGM, float fadeOutDuration)
{
    return _soundManager->Play_BGM(soundFile, volume, stopPrevBGM, fadeOutDuration);
}

bool GameInstance::Play_LoopSound(const wstring& soundFile, ESoundChannel channel, float volume, bool stopPrevChannel,
    float fadeOutDuration)
{
    return _soundManager->Play_Loop(soundFile, channel, volume, stopPrevChannel, fadeOutDuration);
}

void GameInstance::Stop_SoundChannel(ESoundChannel channel, float fadeOutDuration)
{
    _soundManager->Stop_Channel(channel, fadeOutDuration);
}

void GameInstance::Stop_Sound(const wstring& soundFile)
{
    _soundManager->Stop_Sound(soundFile);
}

void GameInstance::Stop_AllSounds(float fadeOutDuration)
{
    _soundManager->Stop_All(fadeOutDuration);
}

void GameInstance::Set_SoundChannelVolume(ESoundChannel channel, float volume)
{
    _soundManager->Set_ChannelVolume(channel, volume);
}

float GameInstance::Get_SoundChannelVolume(ESoundChannel channel) const
{
    return _soundManager->Get_ChannelVolume(channel);
}

bool GameInstance::Has_Sound(const wstring& soundFile) const
{
    return _soundManager->Has_Sound(soundFile);
}

void GameInstance::Add_Collider(Shared<Collider> collider)
{
    return _collisionManager->Add_Collider(collider);
}

void GameInstance::Clear_Colliders()
{
    return _collisionManager->Clear_Colliders();
}

void GameInstance::Draw_DebugBox(const FDebugBoxDesc& desc)
{
    if (_debugManager)
        _debugManager->Draw_Box(desc);
}

void GameInstance::Draw_DebugSphere(const FDebugSphereDesc& desc)
{
    if (_debugManager)
        _debugManager->Draw_Sphere(desc);
}

void GameInstance::Draw_DebugLine(const FDebugLineDesc& desc)
{
    if (_debugManager)
        _debugManager->Draw_Line(desc);
}

void GameInstance::Draw_DebugTraceLine(const FDebugTraceLineDesc& desc)
{
    if (_debugManager)
        _debugManager->Draw_TraceLine(desc);
}

void GameInstance::Draw_DebugMesh(const FDebugMeshDesc& desc)
{
    if (_debugManager)
        _debugManager->Draw_Mesh(desc);
}

void GameInstance::Clear_DebugDraws()
{
    if (_debugManager)
        _debugManager->Clear();
}

void GameInstance::Set_DebugRenderEnabled(bool enabled)
{
    if (_debugManager)
        _debugManager->Set_Enabled(enabled);
}

bool GameInstance::Is_DebugRenderEnabled() const
{
    return _debugManager ? _debugManager->Is_Enabled() : false;
}

HRESULT GameInstance::Render_DebugDepth()
{
    if (_debugManager == nullptr)
        return S_OK;

    return _debugManager->Render_Depth();
}

HRESULT GameInstance::Render_DebugOverlay()
{
    if (_debugManager == nullptr)
        return S_OK;

    return _debugManager->Render_Overlay();
}

#ifdef _DEBUG
void GameInstance::Render_Colliders()
{
    return _collisionManager->Render_Debug();
}
#endif
Shared<Camera> GameInstance::Find_Camera(Protocol::OBJECT_TYPE type)
{
    return _cameraManager->Find_Camera(type);
}

void GameInstance::Clear_InvalidCamera()
{
    return _cameraManager->Clear_InvalidCameras();
}

bool GameInstance::Play_Cinematic(const wstring& sequenceName)
{
    return _cameraManager->Play_Cinematic(sequenceName);
}

bool GameInstance::Play_Cinematic(const wstring& sequenceName, Shared<Transform> anchorTransform, bool blockGameInput)
{
    return _cameraManager->Play_Cinematic(sequenceName, anchorTransform, blockGameInput);
}

void GameInstance::Stop_Cinematic()
{
    return _cameraManager->Stop_Cinematic();
}

void GameInstance::Free()
{
    Base::Free();

    _componentFactory.reset();
    _btNodeFactory.reset();
    _cameraManager.reset();
    _lightManager.reset();
    _soundManager.reset();
    _collisionManager.reset();
    _debugManager.reset();

    _prefabManager.reset();
    _objectManager.reset(); 
    _levelManager.reset();  
    _protoManager.reset();  
    _timerManager.reset();  
    _graphicDevice.reset();
    _textRenderer.reset();
    _animationManager.reset();

    _renderer.reset();
    _pipeLine.reset();

    _uiManager.reset();
    _assetManager.reset();
}
