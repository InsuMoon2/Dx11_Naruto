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
    // Gameplay 청크 로드 직후 collision proxy cache를 다시 빌드해서 즉시 충돌에 반영한다.
    virtual HRESULT On_LevelChunkLoaded(const wstring& fileName) override;

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
    // Konoha와 동일한 스카이박스를 GamePlay 레벨에 생성한다.
    HRESULT         Ready_Layer_SkySphere();
    // 활성 카메라/플레이어 기준으로 shadow 전용 카메라를 다시 맞출 때 호출한다.
    void            Update_DynamicShadowLightFromView();

    static Matrix   Build_CollisionModelPreTransform();

private:
    void            Start_MissionClearSequence();
    void            Update_MissionClearSequence(float timeDelta);
    void            Finish_MissionClearSequence();

private:
    void            Disable_LocalWaveTriggers_ForServerMode();

    void            Spawn_LocalPlayer();
    void            On_PlayerObjectSpawned(Shared<GameObject> obj);

    void            Try_SendEnterGamePacket();

    // 서버에서 로컬 몬스터 제거하게
    void            Remove_LocalMonsters_ForServerMode();

    void            On_WaveStarted(const string& waveTag);
    void            On_WaveCleared(const string& waveTag);
    void            Request_EnterKonoha();


private:
    void            Build_CollisionProxyEntries(vector<FProxyEntry>& outEntries) const;
    HRESULT         Rebuild_CollisionProxyCache();
    void            Apply_CollisionModelsToPlayer(const Shared<GameObject>& obj);

    HRESULT         Collect_CollisionProxyActorsFromLayer(const wstring& layerTag);
    HRESULT         Append_CollisionProxyInstance(Shared<CollisionProxyActor> actor);

    // Gameplay 레벨의 default ground/proxy를 PhysX scene에 등록할 때 호출한다.
    HRESULT         Register_PhysXProxiesForTest();

    // Model instance 하나를 PhysX static triangle mesh로 변환해서 등록할 때 호출한다.
    bool            Register_PhysXProxyInstanceForTest(
                        const MovementComponent::FCollisionModelInstance& instance,
                        const string& debugName,
                        ECollisionProxyType proxyType) const;

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

    FDelegateHandle _waveStartedHandle = {};
    FDelegateHandle _waveClearedHandle = {};

    bool            _konohaTransitionRequested = false;

    bool _showCollisionDebug = false;
    float _collisionModelRefreshAccumulator = 0.f;

    vector<MovementComponent::FCollisionModelInstance> _defaultGroundModels;
    vector<MovementComponent::FCollisionModelInstance> _surfaceProxyModels;
    vector<MovementComponent::FCollisionModelInstance> _worldBlockProxyModels;

    bool  _missionClearSequenceActive = false;
    bool  _missionClearTransitionRequested = false;
    float _missionClearTimer = 0.f;
    float _missionClearHoldTime = 5.f;
    float _missionClearFadeOutTime = 0.8f;

public:
    static Shared<Level_Gameplay> Create(ComPtr<Device> device, ComPtr<DeviceContext> context, EGameplaySpawnMode spawnMode);

    virtual void Free() override;

};

NS_END
