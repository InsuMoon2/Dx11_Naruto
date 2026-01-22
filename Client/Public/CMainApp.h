#pragma once

#include "CBase.h"

NS_BEGIN(Engine)

class CGameInstance;

NS_END

NS_BEGIN(Client)

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
    HRESULT Ready_StartLevel(LEVEL startLevelID);

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    static unique_ptr<CMainApp> Create();
    virtual void Free() override;

};

NS_END
