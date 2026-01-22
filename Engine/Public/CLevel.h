#pragma once

#include "CBase.h"

NS_BEGIN(Engine)

class CGameInstance;

class ENGINE_DLL CLevel abstract : public CBase
{
public:
    explicit CLevel(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CLevel();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta);
    virtual void    LateUpdate(float timeDelta);
    virtual HRESULT Render();

protected:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    virtual void Free() override;

};

NS_END
