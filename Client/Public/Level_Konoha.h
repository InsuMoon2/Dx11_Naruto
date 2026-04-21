#pragma once

#include "Level.h"
#include "MovementComponent.h"
#include "CollisionProxy_Manager.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class Loader;
class UI_PlayerHUD;
class CollisionProxyActor;

class Level_Konoha final : public Level
{
public:
    explicit Level_Konoha(ComPtr<Device> device, ComPtr<DeviceContext> context);
    virtual ~Level_Konoha();

public:
    virtual HRESULT Initialize(EGameplaySpawnMode spawnMode);

    virtual void    Update(float timeDelta) override;
    virtual void    Late_Update(float timeDelta) override;
    virtual HRESULT Render() override;

    static Matrix Build_CollisionModelPreTransform();

private:
    HRESULT         Ready_Lights();
    HRESULT         Ready_Layer_Camera(const wstring& layerTag);
    HRESULT         Ready_Layer_PlayerStart(const wstring& layerTag);
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);

    HRESULT         Ready_Layer_SkySphere();

    HRESULT         Ready_UI();

private:
    void Disable_LocalWaveTriggers_ForServerMode();

    HRESULT Ready_DefaultGroundCollision();
    HRESULT Append_CollisionInstancesFromDirectory(
        const fs::path& directoryPath,
        vector<MovementComponent::FCollisionModelInstance>& outInstances);

    void Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const;

    HRESULT Rebuild_CollisionProxyCache();
    HRESULT Collect_CollisionProxyActorsFromLayer(const wstring& layerTag);
    HRESULT Append_CollisionProxyInstance(Shared<CollisionProxyActor> actor);

   static bool Try_BuildWorldBoundsFromModel(
        Shared<Model> model,
        const Matrix& worldMatrix,
        BoundingBox& outBounds);

private:
    void                    Draw_StaticMeshRender();

    void                    Spawn_LocalPlayer();
    void                    On_PlayerObjectSpawned(Shared<GameObject> obj);

    void                    Try_SendEnterGamePacket();

    // 서버에서 로컬 몬스터 제거하게
    void                    Remove_LocalMonsters_ForServerMode();
    
private:
    Shared<UI_PlayerHUD> _playerHUD;
    FDelegateHandle      _playerObjectSpawnedHandle = {};
                         
    EGameplaySpawnMode   _spawnMode = EGameplaySpawnMode::END;
    bool                 _enterGameSent = false;
    bool                 _showCollisionDebug = false;

    vector<MovementComponent::FCollisionModelInstance> _defaultGroundModels;
    vector<MovementComponent::FCollisionModelInstance> _walkableProxyModels;
    vector<MovementComponent::FCollisionModelInstance> _wallProxyModels;
    vector<MovementComponent::FCollisionModelInstance> _worldBlockProxyModels;

public:
    static Shared<Level_Konoha> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
