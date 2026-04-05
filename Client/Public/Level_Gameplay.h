#pragma once

#include "Level.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class Loader;
class UI_PlayerHUD;

class Level_Gameplay final : public Level
{
public:
    explicit Level_Gameplay(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Gameplay();

public:
    virtual HRESULT Initialize(EGameplaySpawnMode spawnMode);

    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

private:
    HRESULT         Ready_Lights();
    HRESULT         Ready_Layer_Camera(const wstring& layerTag);
    HRESULT         Ready_Layer_PlayerStart(const wstring& layerTag);
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);

    HRESULT         Ready_UI();

    HRESULT         Ready_GroundColliison();

private:
    void            Spawn_LocalPlayer();
    void            On_PlayerObjectSpawned(Shared<GameObject> obj);

    void            Try_SendEnterGamePacket();

private:
    Shared<UI_PlayerHUD> _playerHUD;

    FDelegateHandle     _playerObjectSpawnedHandle = {};

    EGameplaySpawnMode  _spawnMode = EGameplaySpawnMode::END;
    bool                _enterGameSent = false;

    vector<Shared<Model>> _groundCollisionModels;
    vector<Shared<Model>> _wallCollisionModels;

public:
    static Shared<Level_Gameplay> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
