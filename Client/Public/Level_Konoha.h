#pragma once

#include "Level.h"
#include "MovementComponent.h"
#include "CollisionProxy_Manager.h"
#include "Collision_SurfaceCache.h"

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
    virtual HRESULT On_LevelChunkLoaded(const wstring& fileName) override;

    static Matrix Build_CollisionModelPreTransform();

private:
    HRESULT         Ready_Lights();
    HRESULT         Ready_Layer_Camera(const wstring& layerTag);
    HRESULT         Ready_Layer_PlayerStart(const wstring& layerTag);
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);

    HRESULT         Ready_Layer_SkySphere();

    HRESULT         Ready_UI();

    HRESULT         Ready_SurfaceCache();
    HRESULT         Build_SurfaceCacheCellModels();
    fs::path        Build_SurfaceCachePath() const;
    void            Query_SurfaceCacheBoundsModels(
        const BoundingBox& queryBounds,
        vector<MovementComponent::FCollisionModelInstance>& outSurfaceModels,
        vector<MovementComponent::FCollisionModelInstance>& outWorldBlockModels) const;
    Shared<Model>   Find_SurfaceCacheModel(const string& modelGuid);
    static Collision_SurfaceCache::FCellCoord Build_SurfaceCacheCellCoord(
        const Vec3& worldPosition,
        float cellSize);
    static Matrix   Build_SurfaceCacheWorldMatrix(const Collision_SurfaceCache::FEntry& entry);

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
    void                    Refresh_PlayerCollisionModels();
    void                    Refresh_NearbyCollisionModels(float timeDelta);
    void                    Apply_CollisionModelsToPlayer(const Shared<GameObject>& obj);
    void                    On_PlayerObjectSpawned(Shared<GameObject> obj);

    void                    Try_SendEnterGamePacket();

    void                    Remove_LocalMonsters_ForServerMode();
    
private:
    struct FSurfaceCacheCellBucket
    {
        vector<uint32> surfaceIndices;
        vector<uint32> worldBlockIndices;
    };

    Shared<UI_PlayerHUD> _playerHUD;
    FDelegateHandle      _playerObjectSpawnedHandle = {};
                         
    EGameplaySpawnMode   _spawnMode = EGameplaySpawnMode::END;
    bool                 _enterGameSent = false;
    bool                 _showCollisionDebug = false;

    float                _collisionModelRefreshAccumulator = 0.f;

    vector<MovementComponent::FCollisionModelInstance> _defaultGroundModels;
    vector<MovementComponent::FCollisionModelInstance> _surfaceProxyModels;
    vector<MovementComponent::FCollisionModelInstance> _worldBlockProxyModels;

private:
    Unique<Collision_SurfaceCache> _surfaceCache;
    umap<string, Shared<Model>> _surfaceCacheModels;
    std::unordered_map<Collision_SurfaceCache::FCellCoord, FSurfaceCacheCellBucket, Collision_SurfaceCache::FCellCoordHasher> _surfaceCacheCellModels;
    vector<MovementComponent::FCollisionModelInstance> _surfaceCacheSurfaceModels;
    vector<MovementComponent::FCollisionModelInstance> _surfaceCacheWorldBlockModels;

public:
    static Shared<Level_Konoha> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
