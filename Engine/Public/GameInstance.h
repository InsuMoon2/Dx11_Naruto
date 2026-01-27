#pragma once

#include "Base.h"

NS_BEGIN(Engine)
class Graphic_Device;

class Timer_Manager;
class Level_Manager;
class Prototype_Manager;

class Level;

class GameObject;
class Component;

class ENGINE_DLL GameInstance : public Base
{
	DECLARE_SINGLETON(GameInstance)

public:
    explicit GameInstance();
	virtual ~GameInstance();

public:
	HRESULT Initialize_Engine(const ENGINE_DESC& desc, ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);
	void	Update_Engine(float timeDelta);
	void	LateUpdate_Engine(float timeDelta);
	HRESULT Draw();
	void	Clear_Resources(uint32 levelIndex);

public: /* Graphic Device */
	HRESULT Clear_Buffers(const Color& clearColor);
	HRESULT Present();

public: /* Timer Manager */
	HRESULT Add_Timer(const wstring& timerTag);
	float	Compute_TimeDelta(const wstring& timerTag);

public: /* LevelType Manager */
    HRESULT Change_Level(uint32 levelIndex, shared_ptr<Level> level);

public: /* Prototype Manager */
    HRESULT Add_GameObject_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<GameObject> gameObject);
    shared_ptr<GameObject> Clone_GameObject(uint32 levelIndex, const wstring& prototypeTag, void* arg = nullptr);

    HRESULT Add_Component_Prototype(uint32 levelIndex, const wstring& prototypeTag, shared_ptr<Component> component);
    shared_ptr<Component> Clone_Component(uint32 levelIndex, const wstring& prototypeTag, void* arg = nullptr);

private:
	unique_ptr<Graphic_Device>      _graphicDevice;
	unique_ptr<Timer_Manager>	    _timerManager;
    unique_ptr<Level_Manager>       _levelManager;
    unique_ptr<Prototype_Manager>   _protoManager;

public:
	void Free() override;

};

NS_END
