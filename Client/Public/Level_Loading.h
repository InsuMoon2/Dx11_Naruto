#pragma once

#include "Level.h"

NS_BEGIN(Engine)

NS_END

NS_BEGIN(Client)

class Loader;

enum class ELoadingTexture
{
    MainLoading = 0, // 2장
    SpinnerLogo = 2,
    ProgressBar = 3,

    END
};

class Level_Loading final : public Level
{
public:
    explicit Level_Loading(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Loading();

public:
    virtual HRESULT     Initialize(ELevelType nextLevelID, bool loadSharedResources, EGameplaySpawnMode spawnMode);
    virtual void        Update(float timeDelta) override;
    virtual void        Late_Update(float timeDelta) override;
    virtual HRESULT     Render() override;

private:
    HRESULT  Ready_Layer_UI(const wstring& uiTag);

private:
    Shared<UIObject> _loadingBackground;
    Shared<UIObject> _loadingSpinner;
    Shared<UIObject> _loadingProgressBar;

private:
    Shared<Loader>      _loader;
    ELevelType          _nextLevelID = { ELevelType::END };

    bool                _loadSharedResources = false;

    EGameplaySpawnMode  _gameplaySpawnMode = EGameplaySpawnMode::LocalOnly;

public:
    static shared_ptr<Level_Loading> Create(
        ComPtr<Device> device,
        ComPtr<DeviceContext> context,
        ELevelType nextLevelID,
        bool loadSharedResources,
        EGameplaySpawnMode gameplaySpawnMode = EGameplaySpawnMode::LocalOnly);

    virtual void Free() override;

};

NS_END
