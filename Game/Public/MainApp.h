#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class GameInstance;

NS_END

NS_BEGIN(Client)

class MainApp : public Base
{
public:
    // 에디터 멀티플레이 테스트처럼 실행 직후 서버 게임플레이로 진입할지 결정하는 생성자다.
    explicit MainApp(bool startInServerGameplayMode = false);
    virtual ~MainApp();

public:
    HRESULT Initialize();
    void    Priority_Update(float timeDelta);
    void    Update(float timeDelta);
    void    Late_Update(float timeDelta);
    HRESULT Render();

public:
    HRESULT Ready_StaticLevel();
    HRESULT Ready_StartLevel(ELevelType startLevelID);

private:
    ComPtr<Device>              _device;
    ComPtr<DeviceContext>       _context;

private:
    // 에디터에서 Game.exe를 멀티 테스트용으로 실행했는지 기록한다.
    bool _startInServerGameplayMode = false;

public:
    // 실행 모드에 맞는 시작 레벨과 네트워크 초기화를 구성한 MainApp을 생성한다.
    static unique_ptr<MainApp> Create(bool startInServerGameplayMode = false);
    virtual void Free() override;

};

NS_END
