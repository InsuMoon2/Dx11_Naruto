#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

class Level_Loading final : public Level
{
public:
    explicit Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Loading();

public:
    virtual HRESULT Initialize(LevelType nextLevelID);
    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
     virtual HRESULT Render() override;

private:
    shared_ptr<Loader>      _loader;
    LevelType               _nextLevelID = { LevelType::END };

private:
    HRESULT  Ready_Layer_Background(const wstring& layerTag);
    HRESULT  Ready_Layer_UI(const wstring& uiTag);

public:
    static shared_ptr<Level_Loading> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, LevelType nextLevelID);

    virtual void Free() override;

};

NS_END
