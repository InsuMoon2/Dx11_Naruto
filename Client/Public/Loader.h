#pragma once

#include "Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class Loader : public Base, public enable_shared_from_this<Loader>
{
public:
    explicit Loader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Loader();

public:
    HRESULT Initialize(LevelType nextLevelID);
    HRESULT Loading();

    bool    IsFinished()     const { return _isFinished; }
    LevelType   GetNextLevelID() const { return _nextLevelID; }

    #ifdef _DEBUG
    HRESULT Print_LoadingText();
    #endif

private:
    HRESULT Loading_For_LogoLevel();
    HRESULT Loading_For_GamePlay();

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    HANDLE                  _thread = {};
    CRITICAL_SECTION        _criticalSection = {};
    LevelType               _nextLevelID = { LevelType::END };
    bool                    _isFinished = { false };
    tchar                   _loadingText[MAX_PATH] = {};
        
public:
    static shared_ptr<Loader> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, LevelType nextLevelID);

    virtual void Free() override;


};

NS_END
