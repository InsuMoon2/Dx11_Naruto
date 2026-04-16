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

    static Matrix Build_CollisionModelPreTransform();

private:
    void            Spawn_LocalPlayer();
    void            On_PlayerObjectSpawned(Shared<GameObject> obj);

    void            Try_SendEnterGamePacket();

    // 서버에서 로컬 몬스터 제거하게
    void            Remove_LocalMonsters_ForServerMode();

    void            Request_EnterKonoha();

private:
    private:
    static bool             Is_WallCollisionLayerTag(const wstring& layerTag);
    static bool             Is_WallCollisionNameCandidate(const string& candidateName);
    static bool             Is_WallCollisionSizeCandidate(const BoundingBox& bounds);

    static bool             Try_BuildWorldBoundsFromModel(
                                Shared<Model> model,
                                const Matrix& worldMatrix,
                                BoundingBox& outBounds);

    HRESULT                 Rebuild_WallCollisionFromPlacedMeshes();
    HRESULT                 Collect_WallCollisionCandidatesFromLayers(const vector<wstring>& layerTags);
    HRESULT                 Append_WallCollisionInstanceFromActor(Shared<StaticMeshActor> actor);
    Shared<Model>           Get_OrCreateWallCollisionModel(const string& modelGuid, const string& resolvedPath);

    void                    Draw_StaticMeshRender();

private:
    static bool Is_ExtraGroundNameCandidate(const string& candidateName);
    HRESULT Rebuild_ExtraGroundCollisionFromPlacedMeshes();
    HRESULT Append_ExtraGroundCollisionFromActor(Shared<StaticMeshActor> actor);
    
private:
    Shared<UI_PlayerHUD> _playerHUD;

    FDelegateHandle     _playerObjectSpawnedHandle = {};

    EGameplaySpawnMode  _spawnMode = EGameplaySpawnMode::END;
    bool                _enterGameSent = false;

    vector<MovementComponent::FCollisionModelInstance> _groundCollisionModels;
    vector<MovementComponent::FCollisionModelInstance> _wallCollisionModels;
    vector<MovementComponent::FCollisionModelInstance> _extraGroundCollisionModels;

    bool            _konohaTransitionRequested = false;

    umap<string, Shared<Model>> _wallCollisionModelCache;

    bool _showCollisionDebug = false;

public:
    static Shared<Level_Gameplay> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
