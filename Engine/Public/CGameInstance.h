#pragma once

#include "CBase.h"

BEGIN(Engine)
class CGraphic_Device;
class CTimer_Manager;
class CLevel_Manager;
class CLevel;

class ENGINE_DLL CGameInstance : public CBase
{
	DECLARE_SINGLETON(CGameInstance)

public:
    explicit CGameInstance();
	virtual ~CGameInstance();

public:
	HRESULT Initialize_Engine(const ENGINE_DESC& desc, ComPtr<Device>& deviceOut, ComPtr<DeviceContext>& contextOut);
	void	Update_Engine(float timeDelta);
	void	LateUpdate_Engine(float timeDelta);
	HRESULT Draw();
	void	Clear_Resources(uint32 levelIndex);

public: /* Graphic Device */
	HRESULT Clear_Buffers(const color& clearColor);
	HRESULT Present();

public: /* Timer Manager */
	HRESULT Add_Timer(const wstring& timerTag);
	float	Compute_TimeDelta(const wstring& timerTag);

public: /* Level Manager */
    HRESULT Change_Level(uint32 levelIndex, shared_ptr<CLevel> level);

private:
	unique_ptr<CGraphic_Device> _graphicDevice;
	unique_ptr<CTimer_Manager>	_timerManager;
    unique_ptr<CLevel_Manager>  _levelManager;

public:
	void Free() override;

};

END
