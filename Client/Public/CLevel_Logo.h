#pragma once

#include "CLevel.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class CLoader;

class CLevel_Logo final : public CLevel
{
public:
    explicit CLevel_Logo(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~CLevel_Logo();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    LateUpdate(float timeDelta) override;
    virtual HRESULT Render() override;

private:


public:
    static shared_ptr<CLevel_Logo> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
