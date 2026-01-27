#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameInstance;

NS_END

NS_BEGIN(Client)

class MainApp : public Base
{
public:
    explicit MainApp();
    virtual ~MainApp();

public:
    HRESULT Initialize();
    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);
    HRESULT Render();

private:
    HRESULT Ready_StartLevel(LevelType startLevelID);

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    static unique_ptr<MainApp> Create();
    virtual void Free() override;

};

NS_END
