#pragma once

#include "Base.h"

NS_BEGIN(Engine)
/* Device */
class Graphic_Device;

/* Manager */
class Timer_Manager;
class Level_Manager;
class Prototype_Manager;
class Object_Manager;
class Prefab_Manager;

class Renderer;

/* Base */
class GameObject;
class Component;

class Level;

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

	HRESULT                 Draw();
	void	                Clear_Resources(uint32 levelIndex);

public: /* Game State */
    void                    Set_GameState(EGameState state) { _gameState = state; }
    EGameState              Get_GameState() const { return _gameState; }
    bool                    IsPlaying() const { return _gameState == EGameState::Play; }

public: /* Graphic Device */
    ComPtr<Device>          Get_Device();
    ComPtr<DeviceContext>   Get_Context();

    uint32                  Get_ViewportWidth();
    uint32                  Get_ViewportHeight();

    void                    BindBackBuffer();
    HRESULT                 Resize_BackBuffer(uint32 width, uint32 height);

	HRESULT                 Clear_Buffers(const Color& clearColor);
	HRESULT                 Present();

public: /* Timer Manager */
	HRESULT                 Add_Timer(const wstring& timerTag);
	float	                Compute_TimeDelta(const wstring& timerTag);

public: /* LevelType Manager */
    HRESULT                 Change_Level(uint32 levelIndex, shared_ptr<Level> level);
    uint32                  Current_Level();


public: /* Prototype Manager */
    HRESULT                 Add_GameObject_Prototype(uint32 levelIndex, uint32 objID, shared_ptr<GameObject> gameObject);
    shared_ptr<GameObject>  Clone_GameObject(uint32 levelIndex, uint32 objID, void* arg = {});

    HRESULT                 Add_Component_Prototype(uint32 levelIndex, uint32 componentID, shared_ptr<Component> component);
    shared_ptr<Component>   Clone_Component(uint32 levelIndex, uint32 componentID, void* arg = {});

public: /* Object Manager */
    HRESULT                 Add_GameObject(
                                uint32 protoIndex, uint32 objID,
                                uint32 layerIndex, const wstring& layerTag,
                                void* arg = {});

    HRESULT                 Add_GameObject(
                                uint32 levelIndex, uint32 objID,
                                const wstring& layerTag,
                                void* arg = {});

    vector<shared_ptr<GameObject>> Get_GameObjects(uint32 levelIndex);

public: /* Renderer */
    void                    Add_RenderGroup(ERenderGroup renderType, shared_ptr<GameObject> gameObject);
    int32                   Get_DrawCallCount();

public: /* Prefeb */
    Shared<GameObject>      Instantiate_Prefab(const string& prefabName, const json& overrides = {});
    HRESULT                 Save_Prefab(const string& prefabPath, shared_ptr<GameObject> gameObject);

private:
	unique_ptr<Graphic_Device>      _graphicDevice;
	unique_ptr<Timer_Manager>	    _timerManager;
    unique_ptr<Level_Manager>       _levelManager;
    unique_ptr<Prototype_Manager>   _protoManager;
    unique_ptr<Object_Manager>      _objectManager;
    unique_ptr<Prefab_Manager>      _prefabManager;

    unique_ptr<Renderer>            _renderer;

private:
    EGameState                      _gameState = EGameState::Play;

public:
	void Free() override;

};

NS_END
