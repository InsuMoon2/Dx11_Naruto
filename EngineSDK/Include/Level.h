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

    virtual void    On_CharInput(wchar_t ch) {};

public:
    virtual HRESULT Load_LevelFromJson(const wstring& fileName);

    // 단일 오브젝트 JSON을 실제 게임 오브젝트로 만들어 target level에 추가할 때 호출한다.
    static HRESULT  Add_GameObjectJsonToLevel(uint32 targetLevelIndex,
                                              uint32 prototypeLevelIndex,
                                              const json& objJson);

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
