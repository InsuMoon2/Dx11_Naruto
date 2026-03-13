#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameInstance;

class ENGINE_DLL Level abstract : public Base
{
public:
    explicit Level(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level();

public:
    virtual HRESULT Initialize();
    virtual void    Priority_Update(float timeDelta);
    virtual void    Update(float timeDelta);
    virtual void    Late_Update(float timeDelta);
    virtual HRESULT Render();

public:
    virtual HRESULT Load_LevelFromJson(const wstring& fileName);

    static HRESULT  Load_LevelChunkToLevel(uint32 targetLevelIndex,
                                            uint32 prototypeLevelIndex,
                                            const wstring& fileName);

protected:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

public:
    virtual void Free() override;

};

NS_END
