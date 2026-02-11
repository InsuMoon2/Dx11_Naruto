#pragma once

#include "Base.h"

NS_BEGIN(Engine)
class GameInstance;
NS_END

NS_BEGIN(EditorApp)

class Editor_MainApp : public Base
{
public:
    explicit Editor_MainApp();
    virtual ~Editor_MainApp();

public:
    HRESULT Initialize();
    void    Priority_Update(float timeDelta);
    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);
    HRESULT Render();

private:
    HRESULT Ready_StaticLevel();

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    static unique_ptr<Editor_MainApp> Create();
    virtual void Free() override;

};

NS_END
