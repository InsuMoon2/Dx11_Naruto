#pragma once

#include "Base.h"
#include "Client_Defines.h"

NS_BEGIN(Client)

class ResourceLoader;

class Loader : public Base, public enable_shared_from_this<Loader>
{
public:
    explicit Loader(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Loader();

public:
    HRESULT Initialize(ELevelType nextLevelID);
    HRESULT Loading();

    bool    IsFinished()     const { return _isFinished; }
    ELevelType   GetNextLevelID() const { return _nextLevelID; }

    #ifdef _DEBUG
    HRESULT Print_LoadingText();
    #endif

    void    Register_Components();
    void    Initialize_BT_Nodes();

    float   Get_ProgressRatio() const;

private: /* Loading Level */
    HRESULT Loading_For_Maintitle();
    HRESULT Loading_For_GamePlay();

private:
    void    Set_LoadProgress(int currentStep, int totalSteps);

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    HANDLE                  _thread = {};
    CRITICAL_SECTION        _criticalSection = {};
    ELevelType              _nextLevelID = { ELevelType::END };
    bool                    _isFinished = { false };
    tchar                   _loadingText[MAX_PATH] = {};

    Shared<ResourceLoader>  _resourceLoader;

private:
    int     _currentStep    = 0;
    int     _totalSteps     = 0;

public:
    static shared_ptr<Loader> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID);

    virtual void Free() override;


};

NS_END
