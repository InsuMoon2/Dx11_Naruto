#pragma once

#include "Base.h"
#include "Component_Factory.h"
#include "BTNode_Factory.h"
#include "DelegateHub.h"
#include "Prototype_Manager.h"
#include "UI_Manager.h"
#include "CollisionProxy_Manager.h"
#include "PhysXMgr.h"

NS_BEGIN(Engine)
    /* Device */
class Graphic_Device;

/* Manager */
class Timer_Manager;
class Level_Manager;
class Prototype_Manager;
class Object_Manager;
class Prefab_Manager;
class Camera_Manager;
class Light_Manager;
class Asset_Manager;
class UI_Manager;
class Text_Renderer;
class Animation_Manager;
class Sound_Manager;
class Collision_Manager;
class Debug_Manager;
class CollisionProxy_Manager;
class PhysXMgr;
class Target_Manager;
class Shadow;

class Renderer;
class PipeLine;

/* Base */
class GameObject;
class Component;
class VIBuffer_Rect;

class Level;
class Layer;
class Shader;

class Camera;
class UIObject;

class Animation;

class Collider;
struct FDebugBoxDesc;
struct FDebugSphereDesc;
struct FDebugLineDesc;
struct FDebugTraceLineDesc;
struct FDebugMeshDesc;


/* Component */
class Transform;

/* Factory */
class Component_Factory;
class BTNode_Factory;
class GameObject_Factory;

class DelegateHub;

class ENGINE_DLL GameInstance : public Base
{
	DECLARE_SINGLETON(GameInstance)

public:
    explicit GameInstance();
	virtual ~GameInstance();

public:
	HRESULT                 Initialize_Engine(const ENGINE_DESC& desc, ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);

    void                    Priority_Update_Engine(float timeDelta);
	void	                Update_Engine(float timeDelta);
	void	                Late_Update_Engine(float timeDelta);

    // 에디터용 Camera Free만 업데이트
    void                    Update_CameraOnly(float timeDelta);

	HRESULT                 Draw(bool renderDebugPrimitives = true, bool renderColliders = true, bool renderRTDebug = false);
	void	                Clear_Resources(uint32 levelIndex);

public: /* Game State */
    void                    Set_GameState(EGameState state) { _gameState = state; }
    EGameState              Get_GameState() const { return _gameState; }
    bool                    IsPlaying() const { return _gameState == EGameState::Play; }

    void                    Set_GameInputEnabled(bool enabled) { _gameInputEnabled = enabled; }
    bool                    Is_GameInputEnabled() const { return _gameInputEnabled; }

    void                    Set_EditorRuntime(bool enabled) { _editorRuntime = enabled; }
    bool                    Is_EditorRuntime() const { return _editorRuntime; }


public: /* Graphic Device */
    ComPtr<Device>          Get_Device();
    ComPtr<DeviceContext>   Get_Context();

    float                   Get_ViewportWidth();
    float                   Get_ViewportHeight();

    float                   Get_WindowWidth();
    float                   Get_WindowHeight();

    void                    BindBackBuffer();
    HRESULT                 Resize_BackBuffer(uint32 width, uint32 height);

	HRESULT                 Clear_Buffers(const Color& clearColor);
	HRESULT                 Present();

    float                   Get_UIViewportWidth() const;
    float                   Get_UIViewportHeight() const;
    void                    Set_UIViewportSize(float width, float height);

    float                   Get_UIReferenceWidth() const;
    float                   Get_UIReferenceHeight() const;
    void                    Set_UIReferenceSize(float width, float height);

    // UI 비율 유지 스케일 + 중앙 오프셋
    float                    Get_UIScale() const;
    Vec2                     Get_UIViewportOffset() const;

    void                     Clear_DepthOnly();


public: /* ImGui */
    void                    Set_ImGuiContext(void* context);

public: /* Timer Manager */
	HRESULT                 Add_Timer(const wstring& timerTag);
	float	                Compute_TimeDelta(const wstring& timerTag);

public: /* LevelType Manager */
    HRESULT                 Change_Level(uint32 levelIndex, Shared<Level> level);
    uint32                  Current_Level();
    Shared<Level>           Get_Current_Level();

public: /* Prototype Manager */
    HRESULT                 Add_GameObject_Prototype(uint32 levelIndex, uint32 objID, Shared<GameObject> gameObject);
    Shared<GameObject>      Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg = {});

    HRESULT                 Add_Component_Prototype(uint32 levelIndex, uint32 componentID, Shared<Component> component);
    Shared<Component>       Clone_Component(uint32 levelIndex, uint32 componentID, void* arg = {});
    Shared<Component>       Clone_Component(uint32 componentID, void* arg = {});

    Shared<Component>       Find_Component_Prototype(uint32 levelIndex, uint32 componentID);

    vector<pair<uint32, wstring>>  Get_RegisteredGameObjects();

public: /* Object Manager */
    HRESULT                 Add_GameObject(
                                uint32 protoIndex, uint32 objID,
                                uint32 layerIndex, const wstring& layerTag,
                                void* arg = {});

    HRESULT                 Add_GameObject(
                                uint32 levelIndex, uint32 objID,
                                const wstring& layerTag,
                                void* arg = {});

    HRESULT                 Add_GameObject(
                                uint32 levelIndex, const wstring& layerTag,
                                Shared<GameObject> gameObject);

    Shared<GameObject>      Clone_And_Add_GameObject(
                                uint32 protoIndex, uint32 objID,
                                uint32 levelIndex, const wstring& layerTag, void* arg = {});

    vector<Shared<GameObject>>         Get_GameObjects(uint32 levelIndex);
    const umap<wstring, Shared<Layer>> Get_Layers(uint32 levelIndex);

    void                    Clear_Layers(uint32 levelIndex);

    void                    Delete_GameObject(uint32 levelIndex, Shared<GameObject> gameObject);

public: /* Renderer */
    void                    Add_RenderGroup(ERenderGroup renderType, Shared<GameObject> gameObject);
    int32                   Get_DrawCallCount();
    void                    Backup_RenderGroup();
    void                    Restore_RenderGroup();

    HRESULT                 Resize_DeferredViewport(uint32 width, uint32 height);

    HRESULT                 Draw_Preview(bool renderColliders = true);

    void                    Set_BlurStrength(float strength);
    void                    Set_BlurDirection(const Vec2& center);

public: /* Prefeb */
    Shared<GameObject>      Instantiate_Prefab(const string& prefabName, const json& overrides = {});
    HRESULT                 Save_Prefab(const string& prefabPath, Shared<GameObject> gameObject);
    HRESULT                 Load_Prefab(const string& prefabPath);
    void                    Reapply_Prefabs_InCurrentLevel();
    HRESULT                 Reapply_Prefab_ToObject(Shared<GameObject> gameObject);

public: /* PipeLine */
    const Matrix*           Get_Transform(ETransformState state) const;
    const Matrix*           Get_TransformInverse(ETransformState state) const;

    const Vec4*             Get_CamPosition() const;
    void                    Set_Transform(ETransformState state, const Matrix & matrix);

    HRESULT                 Bind_CamPosition(Shared<Shader> shader, const char* constantName);
    HRESULT                 Bind_TransformMatrix(ETransformState state, Shared<Shader> shader, const char* constantName);
    HRESULT                 Bind_TransformMatrix_Inverse(ETransformState state, Shared<Shader> shader, const char* constantName);

public: /* Component_Factory */
    template<typename T>
    void Register_ComponentFactory(uint32 levelIndex)
    {
        _componentFactory->Register<T>(levelIndex, Get_Device(), Get_Context());
    }

    // Shader Texture 등 람다 필요
    void                            Register_ComponentFactory(uint32 typeId, Component_Factory::Creator creator, const wstring& className);
    void                            Register_ComponentFactory_Prototype(uint32 typeId, uint32 levelIndex);
    Shared<Component>               Instantiate_FromFactory(uint32 typeId);

    // 에디터 조회용
    vector<pair<uint32, wstring>>   Get_RegisteredComponents();

public: /* GameObejct Factory */
    Shared<GameObject>              Create_GameObjectFromFactory(Protocol::OBJECT_TYPE type);

public: /* BTNode_Factory */
    void                            Register_BTNode(const string& category, const string& typeName, BTNode_Factory::Creator creator);
    Shared<BTNode>                  Instantiate_BTNode(const string& typeName);
    const umap<string, BTNode_Factory::NodeInfo>& Get_RegisteredBTNodes() const;

public: /* Camera */
    void                            Set_ActiveCamera(Shared<Camera> camera);
    Shared<Camera>                  Get_ActiveCamera();
    bool                            Is_ActiveCamera(Shared<Camera> camera);

    void                            Toggle_Camera();
    void                            Register_Camera(Shared<Camera> camera);

    Shared<Camera>                  Find_Camera(Protocol::OBJECT_TYPE type);

    void                            Clear_InvalidCamera();

    /* 시네마틱 */
    bool                            Play_Cinematic(const wstring& sequenceName);
    bool                            Play_Cinematic(
                                        const wstring& sequenceName,
                                        Shared<Transform> anchorTransform,
                                        bool blockGameInput);
    bool                            Is_CinematicPlaying() const;

    void                            Stop_Cinematic();


    void                            Request_CameraShake(const FCameraShakeDesc& request);
    void                            Stop_CameraShake(const string& tag = "");
    void                            Clear_CameraShake();


public: /* DelegateHub */
    DelegateHub&                    Get_DelegateHub() { return _delegateHub; }


public: /* Light */
    const FLightDesc*               Get_LightDesc(uint32 index);
    HRESULT                         Add_Light(const FLightDesc& desc);
    // 현재 primary shadow directional light의 설정을 교체할 때 호출한다.
    HRESULT                         Update_PrimaryShadowLightDesc(const FLightDesc& desc);
    void                            Clear_Lights();

    HRESULT                         Render_Lights(Shared<Shader> shader, Shared<VIBuffer_Rect> viBuffer);

    const                           FLightDesc* Get_PrimaryShadowLightDesc() const;
    // 에디터가 primary shadow light 자동 갱신을 잠그고 수동 제어를 시작하거나 끝낼 때 호출한다.
    void                            Set_EditorPrimaryShadowLightOverrideEnabled(bool enabled);
    // 에디터가 현재 primary shadow light를 수동 제어 중인지 확인할 때 호출한다.
    bool                            Is_EditorPrimaryShadowLightOverrideEnabled() const { return _editorPrimaryShadowLightOverrideEnabled; }
    // 에디터 인스펙터에서 편집한 primary shadow light 설정을 즉시 반영할 때 호출한다.
    HRESULT                         Set_EditorPrimaryShadowLightOverrideDesc(const FLightDesc& desc);
    // 에디터가 현재 수동 제어 중인 primary shadow light 설정을 읽을 때 호출한다.
    const FLightDesc*               Get_EditorPrimaryShadowLightOverrideDesc() const;

public: /* Asset */
    string                          Find_AssetGUID(const wstring& filePath);
    const FAssetMeta*               Find_AssetByGUID(const string& guid);
    wstring                         Resolve_AssetPath(const string& guid);
    string                          Register_Asset(const wstring& filePath, const string& type = "");
    void                            Scan_Assets(const wstring& directory);
    vector<const FAssetMeta*>       Get_AssetByType(const string& type);
    void                            Update_AssetPath(const string& guid, const wstring& newFilePath);
    void                            Refresh_Cache();
    uint32                          Clear_DisallowedMeta(const wstring& directory);

public: /* UI */
    Shared<UIObject>                Find_UI(const wstring& name);
    void                            Show_UI(const wstring& name);
    void                            Hide_UI(const wstring& name);
    void                            Toggle_UI(const wstring& name);
    void                            Hide_All_Layer(EUILayer layer);
    void                            Hide_All_UI();
    bool                            Is_UIInputBlocked() const;
    void                            Clear_UI();
    void                            Clear_UI_ByLevel(uint32 levelIndex);

    const list<Shared<UIObject>>&   Get_UILayers(EUILayer layer) const;

	Shared<UIObject>                Add_UI(uint32 objID, EUILayer layer, void* arg = {});
    Shared<UIObject>                Clone_UI(uint32 objID, void* arg = {});
    HRESULT                         Register_UI(EUILayer layer, Shared<UIObject> uiObject);
    void                            Set_UIPrototypeLevel(uint32 levelIndex);

    HRESULT                         Remove_UI(const wstring& name);
    HRESULT                         Remove_UI(const Shared<UIObject>& uiObject);

public: /* UI Animation */
    bool                            Play_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                            Play_UIAnimation(const wstring& targetName, const string& animationName);

    bool                            Pause_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                            Pause_UIAnimation(const wstring& targetName, const string& animationName);

    bool                            Stop_UIAnimation(Shared<UIObject> target, const string& animationName);
    bool                            Stop_UIAnimation(const wstring& targetName, const string& animationName);

public: /* Text Renderer */
    HRESULT Begin_UIText();
    HRESULT Draw_Text(const wstring& text, const RECT& rect, const FTextStyle& style);
    HRESULT End_UIText();

    HRESULT Set_TextTarget_Texture(ComPtr<Texture2D> texture);
    HRESULT Reset_TextTarget_BackBuffer();

public: /* Animation Manager */
    void                Load_Animations_From_Directory(const wstring& directoryPath);
    Shared<Animation>   Get_Animation(const string& name) const;

    vector<Shared<Animation>> Get_Animations_By_Prefix(const string& prefix) const;
    vector<Shared<Animation>> Get_All_Animations();

    vector<Shared<Animation>> Get_Animations_InFolder(const string& folderPath);

public: /* Sound Manager */
    bool                    Play_Sound(const wstring& soundFile, ESoundChannel channel, float volume = 1.f);
    bool                    Play_Sound_Pitched(const wstring& soundFile, ESoundChannel channel, float volume, float pitch);

    bool                    Play_BGM(const wstring& soundFile,
        float volume = 1.f,
        bool stopPrevBGM = true,
        float fadeOutDuration = 0.f);

    bool                    Play_LoopSound(const wstring& soundFile,
        ESoundChannel channel,
        float volume = 1.f,
        bool stopPrevChannel = false,
        float fadeOutDuration = 0.f);

    void                    Stop_SoundChannel(ESoundChannel channel, float fadeOutDuration = 0.f);
    void                    Stop_Sound(const wstring& soundFile);
    void                    Stop_AllSounds(float fadeOutDuration = 0.f);

    void                    Set_SoundChannelVolume(ESoundChannel channel, float volume);
    float                   Get_SoundChannelVolume(ESoundChannel channel) const;

    bool                    Has_Sound(const wstring& soundFile) const;

public: /* Collision */
    void                    Add_Collider(Shared<Collider> collider);
    void                    Clear_Colliders();

#ifdef _DEBUG
    void                    Render_Colliders();
#endif

public: /* Debug */
    void                    Draw_DebugBox(const FDebugBoxDesc& desc);
    void                    Draw_DebugSphere(const FDebugSphereDesc& desc);
    void                    Draw_DebugLine(const FDebugLineDesc& desc);
    void                    Draw_DebugTraceLine(const FDebugTraceLineDesc& desc);
    void                    Draw_DebugMesh(const FDebugMeshDesc& desc);

    void                    Clear_DebugDraws();
    void                    Set_DebugRenderEnabled(bool enabled);

    bool                    Is_DebugRenderEnabled() const;

    HRESULT                 Render_DebugDepth();
    HRESULT                 Render_DebugOverlay();

public: /* Collision Proxy */
    void Ready_CollisionProxy(const vector<FProxyEntry>& entries);

    void Query_ActiveCollisionProxy(
        const Vec3& focusPos,
        vector<MovementComponent::FCollisionModelInstance>& outWalkable,
        vector<MovementComponent::FCollisionModelInstance>& outWall,
        vector<MovementComponent::FCollisionModelInstance>* outWorldBlock = nullptr) const;

    void Clear_CollisionProxy();

public: /* PhysX */
    // 레벨 로드 중 충돌 전용 메시를 PhysX static triangle mesh로 등록할 때 호출한다.
    bool Register_PhysXStaticTriangleMesh(
        const string& name,
        const vector<Vec3>& vertices,
        const vector<uint32>& indices,
        const Matrix& worldMatrix,
        ECollisionProxyType proxyType);
    // Movement/Editor 디버그에서 PhysX scene 기준 raycast를 확인할 때 호출한다.
    bool Raycast_PhysX(
        const Vec3& origin,
        const Vec3& direction,
        float distance,
        FPhysXRaycastHit& outHit,
        ECollisionProxyType requiredProxyType = ECollisionProxyType::END) const;
    // 레벨 전환 또는 PhysX 충돌 데이터 재구축 전에 등록된 static geometry를 비울 때 호출한다.
    void Clear_PhysXScene();

public: /* Target Manager */
    HRESULT     Add_RenderTarget(const wstring& targetTag, uint32 sizeX, uint32 sizeY, DXGI_FORMAT format, const Color& clearColor, bool resizeWithViewport = true);
    HRESULT     Add_MRT(const wstring& mrtTag, const wstring& targetTag);

    HRESULT     Copy_CurrentRenderTargetToRT(const wstring& targetTag);

    HRESULT     Begin_MRT(const wstring& mrtTag, ComPtr<DepthStencil> customDSV = nullptr);
    HRESULT     End_MRT();
    HRESULT     Bind_RT_ShaderResource(Shared<Shader> shader, const char* constantName, const wstring& targetTag);
    HRESULT     Resize_MRTs(uint32 width, uint32 height);
#ifdef _DEBUG
    HRESULT     Ready_RT_Debug(const wstring& targetTag, float x, float y, float sizeX, float sizeY);
    HRESULT     Render_RT_Debug(Shared<VIBuffer_Rect> viBuffer, Shared<Shader> shader, const wstring& mrtTag);

    // Debug shadow/RT 진단용: 지정 RT의 float 픽셀 통계를 로그로 찍어 shadow pass 결과를 확인한다.
    HRESULT     Log_RT_DebugFloatStats(const wstring& targetTag);
#endif

public:
    // 현재 primary directional light를 shadow 클래스에 동기화할 때 호출한다.
    bool Update_PrimaryShadowLight();
    // deferred 합성이나 shadow 리소스 준비에 쓰는 현재 shadow light desc다.
    const FLightDesc* Get_ShadowLightDesc() const;
    HRESULT Bind_ShadowMatrices(
        Shared<Shader> shader,
        const char* viewName,
        const char* projName);

    const Matrix* Get_ShadowViewMatrix() const;
    const Matrix* Get_ShadowProjMatrix() const;
    void Invalidate_StaticShadowMap();

private: /* Manager */
	Unique<Graphic_Device>          _graphicDevice  {};
	Unique<Timer_Manager>	        _timerManager   {};
    Unique<Level_Manager>           _levelManager   {};
    Unique<Prototype_Manager>       _protoManager   {};
    Unique<Object_Manager>          _objectManager  {};
    Unique<Prefab_Manager>          _prefabManager  {};
    Unique<Camera_Manager>          _cameraManager  {};
    Unique<Light_Manager>           _lightManager   {};
    Unique<Asset_Manager>           _assetManager   {};
    Unique<UI_Manager>              _uiManager      {};
    Unique<Text_Renderer>           _textRenderer   {};
    Unique<Animation_Manager>       _animationManager{};
    Unique<Sound_Manager>           _soundManager   {};
    Unique<Collision_Manager>       _collisionManager{};
    Unique<Debug_Manager>           _debugManager{}; // 충돌 여부와 무관한 디버그 도형 요청을 모아 렌더하는 매니저
    Unique<CollisionProxy_Manager>  _collisionProxyManager{};
    Unique<PhysXMgr>                _physXMgr{}; // PhysX 충돌 scene과 static triangle mesh를 관리하는 실험용 매니저
    Unique<Target_Manager>          _targetManager;
    Unique<Shadow>                  _shadow{}; // 수업코드 스타일로 분리한 shadow 행렬/설정 소유자다.

    Unique<Renderer>                _renderer {};
    Unique<PipeLine>                _pipeLine {};

    

private: /* Factory */
    Unique<Component_Factory>       _componentFactory {};
    Unique<BTNode_Factory>          _btNodeFactory    {};
    Unique<GameObject_Factory>      _gameObjectFactory{};

private: /* Delegate Hub */
    DelegateHub                     _delegateHub;

private:
    EGameState                      _gameState = EGameState::Play;
    bool                            _gameInputEnabled = false;
    bool                            _editorRuntime = false;
    bool                            _isCinematicTransition = false;

public:
    void                            Set_CinematicTransition(bool b) { _isCinematicTransition = b; }
    bool                            Is_CinematicTransition() const { return _isCinematicTransition; }
    // 에디터가 primary shadow light를 잠가서 레벨 코드의 자동 업데이트보다 우선 적용할지 결정하는 스위치다.
    bool                            _editorPrimaryShadowLightOverrideEnabled = false;
    // 에디터 인스펙터에서 직접 조정 중인 primary shadow light의 최신 설정이다.
    FLightDesc                      _editorPrimaryShadowLightOverrideDesc{};

private:
    float                           _uiViewportWidth = 0.f;
    float                           _uiViewportHeight = 0.f;

    float                           _uiReferenceHeight = 1080.f;
    float                           _uiReferenceWidth = 1920.f;


public:
	void Free() override;

};

NS_END
