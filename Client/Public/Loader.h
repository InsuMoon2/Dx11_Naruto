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
    HRESULT Initialize(ELevelType nextLevelID);
    HRESULT Loading();

    bool    IsFinished()     const { return _isFinished; }
    ELevelType   GetNextLevelID() const { return _nextLevelID; }

    #ifdef _DEBUG
    HRESULT Print_LoadingText();
    #endif

    void    Register_Components();
    void    Initialize_BT_Nodes();

private: /* Loading Level */
    HRESULT Loading_For_LogoLevel();
    HRESULT Loading_For_GamePlay();

private: /* json Data */ 
    HRESULT Load_Resources_From_Json(const wstring& filePath);
    uint32  Get_LevelIndex_From_String(const wstring& levelName);
    uint32  Get_ComponentID_From_String(const wstring& key);

private:
    ComPtr<Device>          _device;
    ComPtr<DeviceContext>   _context;

    HANDLE                  _thread = {};
    CRITICAL_SECTION        _criticalSection = {};
    ELevelType              _nextLevelID = { ELevelType::END };
    bool                    _isFinished = { false };
    tchar                   _loadingText[MAX_PATH] = {};
        
public:
    static shared_ptr<Loader> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID);

    virtual void Free() override;


};

NS_END
