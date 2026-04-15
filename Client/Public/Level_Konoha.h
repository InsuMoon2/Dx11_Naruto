#pragma once

#include "Level.h"
#include "MovementComponent.h"

NS_BEGIN(Engine)
class Model;
NS_END

NS_BEGIN(Client)

class Loader;
class UI_PlayerHUD;
class StaticMeshActor;


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
    static bool Try_BuildWorldBoundsFromModel(
        Shared<Model> model,
        const Matrix& worldMatrix,
        BoundingBox& outBounds);

private:
    HRESULT         Ready_Lights();
    HRESULT         Ready_Layer_Camera(const wstring& layerTag);
    HRESULT         Ready_Layer_PlayerStart(const wstring& layerTag);
    HRESULT         Ready_Layer_GameObject(const wstring& layerTag);

    HRESULT         Ready_UI();

    HRESULT         Ready_GroundColliison();
    HRESULT         Append_CollisionInstancesFromDirectory(
                        const string& dirPath,
                        bool treatAsWall);

private:
    // 벽 충돌 판정 확인
    static bool             Is_WallCollisionLayerTag(const wstring& layerTag);
    static bool             Is_WallCollisionNameCandidate(const string& candidateName);

    // 작은 애들은 제거
    static bool             Is_WallCollisionSizeCandidate(const BoundingBox& bounds);

    // 레벨에 실제 배치된 벽 충돌체 다시 만들기
    HRESULT                 Rebuild_WallCollisionFromPlacedMeshes();
    HRESULT                 Append_WallCollisionInstanceFromActor(Shared<StaticMeshActor> actor);

    Shared<Model>           Get_OrCreateWallCollisionModel(const string& modelGuid, const string& resolvedPath);

    HRESULT                 Collect_WallCollisionCandidatesFromLayers(const vector<wstring>& layerTags);

private:
    void                    Draw_StaticMeshRender();

    void                    Spawn_LocalPlayer();
    void                    On_PlayerObjectSpawned(Shared<GameObject> obj);

    void                    Try_SendEnterGamePacket();

    // 서버에서 로컬 몬스터 제거하게
    void                    Remove_LocalMonsters_ForServerMode();
    
private:
    Shared<UI_PlayerHUD> _playerHUD;

    FDelegateHandle     _playerObjectSpawnedHandle = {};

    EGameplaySpawnMode  _spawnMode = EGameplaySpawnMode::END;
    bool                _enterGameSent = false;

    vector<MovementComponent::FCollisionModelInstance> _groundCollisionModels;
    vector<MovementComponent::FCollisionModelInstance> _wallCollisionModels;

    umap<string, Shared<Model>> _wallCollisionModelCache;

     bool               _showCollisionDebug = false;

public:
    static Shared<Level_Konoha> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
