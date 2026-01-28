#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

class Level_Logo final : public Level
{
public:
    explicit Level_Logo(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Logo();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_Background(const wstring& layerTag);


public:
    static shared_ptr<Level_Logo> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
