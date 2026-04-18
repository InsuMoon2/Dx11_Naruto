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

    // ExamStadium 기본 바닥 collision mesh를 읽어 replacement/ground 판정에 사용할 준비를 한다.
    HRESULT         Ready_DefaultGroundCollision();

    // 지정한 디렉터리에서 ExamStadium 바닥 collision 후보 meshbin을 읽어 cache에 추가한다.
    HRESULT         Append_CollisionInstancesFromDirectory(
                        const fs::path& directoryPath,
                        vector<MovementComponent::FCollisionModelInstance>& outInstances);

    HRESULT         Ready_UI();

    static Matrix Build_CollisionModelPreTransform();

private:
    void            Spawn_LocalPlayer();
    void            On_PlayerObjectSpawned(Shared<GameObject> obj);

    void            Try_SendEnterGamePacket();

    // 서버에서 로컬 몬스터 제거하게
    void            Remove_LocalMonsters_ForServerMode();

    void            Request_EnterKonoha();

private:
    // Gameplay 레벨에서 사용할 ground/world-block collision entry를 manager 형식으로 구성한다.
    void            Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const;

    // 레벨에 배치된 CollisionProxyActor를 다시 읽어 proxy cache를 재구성한다.
    HRESULT         Rebuild_CollisionProxyCache();

    // 지정한 layer에서 collision proxy actor를 수집한다.
    HRESULT         Collect_CollisionProxyActorsFromLayer(const wstring& layerTag);

    // proxy type에 따라 walkable / wall / world block cache에 분류해 넣는다.
    HRESULT         Append_CollisionProxyInstance(Shared<CollisionProxyActor> actor);

    // proxy model의 월드 bounds를 계산해 broad phase에 사용할 수 있게 만든다.
    static bool     Try_BuildWorldBoundsFromModel(
                        Shared<Model> model,
                        const Matrix& worldMatrix,
                        BoundingBox& outBounds);

    void            Draw_StaticMeshRender();
    
private:
    Shared<UI_PlayerHUD> _playerHUD;

    FDelegateHandle     _playerObjectSpawnedHandle = {};

    EGameplaySpawnMode  _spawnMode = EGameplaySpawnMode::END;
    bool                _enterGameSent = false;

    bool            _konohaTransitionRequested = false;

    bool _showCollisionDebug = false;

    // ExamStadium 기본 바닥 판정에 사용하는 ground collision cache다.
    vector<MovementComponent::FCollisionModelInstance> _defaultGroundModels;

    // 플레이어 ground 판정에 사용하는 walkable proxy cache다.
    vector<MovementComponent::FCollisionModelInstance> _walkableProxyModels;

    // wall-run / wire dash 판정에 사용하는 wall proxy cache다.
    vector<MovementComponent::FCollisionModelInstance> _wallProxyModels;

    // 2차에서 world block 충돌용으로 확장할 proxy cache다.
    vector<MovementComponent::FCollisionModelInstance> _worldBlockProxyModels;

public:
    static Shared<Level_Gameplay> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
