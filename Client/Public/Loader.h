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
    HRESULT Initialize(ELevelType nextLevelID, bool loadSharedResources);
    HRESULT Loading();

    bool         IsFinished()     const { return _isFinished; }
    ELevelType   GetNextLevelID() const { return _nextLevelID; }

    #ifdef _DEBUG
    HRESULT Print_LoadingText();
    #endif

    void    Register_Components();
    void    Initialize_BT_Nodes();

    float   Get_ProgressRatio() const;

public: /* Thread */
    bool    Pop_NextJob(FLoadJob& outJob);
    HRESULT Execute_Job_OnMainThread(const FLoadJob& job);

    bool    Is_PrepareFinished() const { return _prepareFinished.load(); }
    bool    Has_PrepareFailed() const { return _prepareFailed.load(); }

private: /* Loading Level */
    HRESULT Loading_For_Maintitle();
    HRESULT Loading_For_GamePlay();


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
    mutex               _jobMutex;
    queue<FLoadJob>     _pendingJobs;

    atomic<int32>      _totalJobs = 0;
    atomic<int32>      _completedJobs = 0;

    atomic<bool>       _prepareFinished = false;
    atomic<bool>       _prepareFailed = false;

    bool               _loadSharedResources = false;

public:
    static shared_ptr<Loader> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context,
        ELevelType nextLevelID, bool loadSharedResources = false);

    virtual void Free() override;


};

NS_END
