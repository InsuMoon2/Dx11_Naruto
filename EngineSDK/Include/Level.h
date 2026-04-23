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
    // 레벨 청크(.level.json / .proxy.level.json)가 현재 레벨에 추가된 직후 후처리가 필요할 때 호출된다.
    // CollisionProxy cache 재빌드처럼 "로드는 끝났지만 런타임 캐시는 아직 옛 상태"인 문제를 여기서 정리한다.
    virtual HRESULT On_LevelChunkLoaded(const wstring& fileName) { return S_OK; }

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
