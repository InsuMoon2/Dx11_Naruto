#pragma once

#include "CLevel.h"

BEGIN(Engine)

END

BEGIN(Client)

class CLevel_Loading final : public CLevel
{
public:
    explicit CLevel_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CLevel_Loading();

public:
    virtual HRESULT Initialize() override;
    virtual void    Update(float timeDelta) override;
    virtual void    LateUpdate(float timeDelta) override;
    virtual HRESULT Render() override;

public:
    static shared_ptr<CLevel_Loading> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual void Free() override;

};

END
