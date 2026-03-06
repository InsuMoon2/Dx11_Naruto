#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

enum class EMainTitle
{
    BG_0, BG_1,
    Logo,
    Text0, Text1, Text2
};

class Level_MainTitle final : public Level
{
public:
    explicit Level_MainTitle(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_MainTitle();

public:
    virtual HRESULT Initialize();
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Layer_Background();

public:
    static shared_ptr<Level_MainTitle> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);

    virtual void Free() override;

};

NS_END
