#pragma once

#include "CBase.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class CLoader : public CBase, public enable_shared_from_this<CLoader>
{
public:
    explicit CLoader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CLoader();

public:
    HRESULT Initialize(LEVEL nextLevelID);
    HRESULT Loading();

    bool    IsFinished() const { return _isFinished; }
    LEVEL   GetNextLevelID() const { return _nextLevelID; }

private:
    HRESULT Loading_For_LogoLevel();
    HRESULT Loading_For_GamePlay();

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    HANDLE                  _thread = {};
    CRITICAL_SECTION        _criticalSection = {};
    LEVEL                   _nextLevelID = { LEVEL::END };
    bool                    _isFinished = { false };

public:
    static shared_ptr<CLoader> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, LEVEL nextLevelID);

    virtual void Free() override;


};

NS_END
