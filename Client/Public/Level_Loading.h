#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

enum class ELoadingTexture
{
    MainLoading,
};

class Level_Loading final : public Level
{
public:
    explicit Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Loading();

public:
    virtual HRESULT     Initialize(ELevelType nextLevelID);
    virtual void        Update(float timeDelta) override;
    virtual void        Late_Update(float timeDelta) override;
     virtual HRESULT    Render() override;

private:
    shared_ptr<Loader>      _loader;
    ELevelType              _nextLevelID = { ELevelType::END };

private:
    HRESULT  Ready_Layer_UI(const wstring& uiTag);

public:
    static shared_ptr<Level_Loading> Create(
        ComPtr<Device> device, ComPtr<DeviceContext> context, ELevelType nextLevelID);

    virtual void Free() override;

};

NS_END
