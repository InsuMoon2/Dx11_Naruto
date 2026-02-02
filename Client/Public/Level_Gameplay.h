#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

class Level_Gameplay final : public Level
{
public:
    explicit Level_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Gameplay();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);
    HRESULT         Ready_Layer_TempLayer(const wstring& layerTag);


public:
    static shared_ptr<Level_Gameplay> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
