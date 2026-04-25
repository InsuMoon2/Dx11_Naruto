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
    virtual HRESULT On_LevelChunkLoaded(const wstring& fileName) override;

    static Matrix Build_CollisionModelPreTransform();

private:
    struct FCollisionCellCoord
    {
        int32 x = 0;
        int32 z = 0;

        bool operator==(const FCollisionCellCoord& rhs) const
        {
            return x == rhs.x && z == rhs.z;
        }
    };

    struct FCollisionCellCoordHasher
    {
        size_t operator()(const FCollisionCellCoord& key) const
        {
            const size_t hx = std::hash<int32>{}(key.x);
            const size_t hz = std::hash<int32>{}(key.z);
            return hx ^ (hz << 1);
        }
    };

    struct FCollisionCellBucket
    {
        vector<uint32> surfaceIndices;
        vector<uint32> worldBlockIndices;
    };

    HRESULT         Ready_Lights();
    HRESULT         Ready_Layer_Camera(const wstring& layerTag);
    HRESULT         Ready_Layer_PlayerStart(const wstring& layerTag);
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);

    HRESULT         Ready_Layer_SkySphere();

    HRESULT         Ready_UI();
    // 활성 카메라/플레이어 기준으로 shadow 전용 카메라를 다시 맞출 때 호출한다.
    void            Update_DynamicShadowLightFromView();

    HRESULT         Build_CollisionProxyCellModels();
    void            Query_CollisionProxyBoundsModels(
        const BoundingBox& queryBounds,
        vector<MovementComponent::FCollisionModelInstance>& outSurfaceModels,
        vector<MovementComponent::FCollisionModelInstance>& outWorldBlockModels) const;

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
    // Konoha proxy들을 PhysX scene에 등록해서 좌표/normal 검증용으로 사용한다.
    HRESULT Register_PhysXProxiesForTest(const Vec3& playerStartPos);
    // proxy/model 인스턴스 하나를 PhysX triangle mesh 입력 배열로 합쳐 등록할 때 사용한다.
    bool Register_PhysXProxyInstanceForTest(
        const MovementComponent::FCollisionModelInstance& instance,
        const string& debugName,
        ECollisionProxyType proxyType) const;

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
    
    Shared<UI_PlayerHUD> _playerHUD;
    FDelegateHandle      _playerObjectSpawnedHandle = {};
                         
    EGameplaySpawnMode   _spawnMode = EGameplaySpawnMode::END;
    bool                 _enterGameSent = false;
    bool                 _showCollisionDebug = false;

    float                _collisionModelRefreshAccumulator = 0.f;

    vector<MovementComponent::FCollisionModelInstance> _defaultGroundModels;
    vector<MovementComponent::FCollisionModelInstance> _surfaceProxyModels;
    vector<MovementComponent::FCollisionModelInstance> _worldBlockProxyModels;
    vector<ECollisionProxyType> _surfaceProxyTypes; // _surfaceProxyModels와 같은 index로 Walkable/WallRun 타입을 보관한다.
    Vec3 _physXTestProxyCenter = Vec3::Zero; // PlayerStart 주변에 등록한 PhysX 테스트 proxy 중 가장 가까운 중심 위치다.
    bool _hasPhysXTestProxy = false; // F3 디버그에서 테스트 proxy 방향 ray를 그릴지 판단한다.

private:
    std::unordered_map<FCollisionCellCoord, FCollisionCellBucket, FCollisionCellCoordHasher> _collisionProxyCellModels;
    float _collisionProxyCellSize = 10.f;

public:
    static Shared<Level_Konoha> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
