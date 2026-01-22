#pragma once

#include "CLevel.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class CLoader;

class CLevel_Loading final : public CLevel
{
public:
    explicit CLevel_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CLevel_Loading();

public:
    virtual HRESULT Initialize(LEVEL nextLevelID);
    virtual void    Update(float timeDelta) override;
    virtual void    LateUpdate(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    shared_ptr<CLoader> _loader;

private:
    HRESULT Ready_Layer_Background(const wstring& layerTag);
    HRESULT Ready_Layer_UI(const wstring& uiTag);

public:
    static shared_ptr<CLevel_Loading> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, LEVEL nextLevelID);

    virtual void Free() override;

};

NS_END
