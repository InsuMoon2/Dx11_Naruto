#pragma once

#include "CBase.h"

BEGIN(Engine)

class CGameInstance;

END

BEGIN(Client)

class CMainApp : public CBase
{
public:
    explicit CMainApp();
    virtual ~CMainApp();

public:
    HRESULT Initialize();
    void    Update(float timeDelta);
    void    LateUpdate(float timeDelta);
    HRESULT Render();

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    static unique_ptr<CMainApp> Create();
    virtual void Free() override;

};

END
