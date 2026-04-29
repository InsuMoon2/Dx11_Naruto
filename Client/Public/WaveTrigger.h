#pragma once

#include "GameObject.h"

NS_BEGIN(Engine)
class EffectComponent;
NS_END

NS_BEGIN(Client)

class WaveTrigger : public GameObject
{
    GENERATED_BODY(WaveTrigger)

public:
    struct FWaveSpawnEntry
    {
        string  prefabName; // 어떤 프리펩을 소환할건지 ? 프리펩만 가능하게 해도 될듯
        Vec3    position = Vec3::Zero;
        Vec3    rotation = Vec3::Zero;
        Vec3    scale    = Vec3::One;

        uint32 objectTypeOverride = ETOI(Protocol::OBJECT_TYPE_MONSTER); // 기본 몬스터, 다른 몹으로 바꿀 수 있게도 나중에 보스?
    };

    struct FPrewarmedSpawnEntry
    {
        Shared<GameObject> monster = nullptr;
        FWaveSpawnEntry source;
        bool isSpawned = false;
    };

public:
    explicit WaveTrigger(ComPtr<Device> device, ComPtr<DeviceContext> context);
    explicit WaveTrigger(const WaveTrigger& rhs);
    virtual ~WaveTrigger() = default;

public:
    HRESULT Initialize_Prototype() override;
    HRESULT Initialize(void* arg) override;

    void    BeginPlay() override;
    void    Update(float timeDelta) override;
    void    Late_Update(float timeDelta) override;
    HRESULT Render() override;

    json    To_Json() const override;
    void    From_Json(const json& data) override;

    void    OnBeginOverlap(Shared<Collider> self, Shared<Collider> other) override;

public:
    vector<FWaveSpawnEntry>& Get_SpawnEntries() { return _spawnEntries; }
    const vector<FWaveSpawnEntry>& Get_SpawnEntries() const { return _spawnEntries; }
    void Refresh_ForEditorPlay();

    void Set_ServerAuthoritative(bool enabled);
    bool Is_ServerAuthoritative() const { return _serverAuthoritative; }

private:
    HRESULT Ready_Components();

    /** 플레이어가 트리거에 진입했을 때 실제 웨이브 시작 가능 상태인지 검사하고 스폰을 시작한다. */
    void    Try_TriggerWave(Shared<GameObject> otherObject);
    /** 레벨 시작 시 웨이브 몬스터를 미리 생성해두고, 실제 시작 때 월드에 배치할 준비를 한다. */
    void    Prewarm_Wave();
    /** 프리웜된 몬스터들을 실제 월드에 배치해서 현재 웨이브를 시작한다. */
    void    Spawn_Wave();
    /** 현재 웨이브의 생존 몬스터를 추적해서 모두 죽었는지 감시한다. */
    void    Update_SpawnedMonsters(float timeDelta);
    /** 현재 웨이브 종료 처리와 클리어 브로드캐스트를 수행한다. */
    void    Finish_WaveClear();
    /** 선행 웨이브가 클리어되면 잠겨 있던 다음 웨이브 트리거를 해금한다. */
    void    Handle_RequiredWaveCleared(const string& clearedWaveTag);

    /** 현재 오브젝트가 추적 중인 몬스터가 사망 처리되었는지 판정한다. */
    bool    Is_TrackedMonsterDead(const Shared<GameObject>& obj) const;

private:
    Shared<Collider>            _triggerCollider = nullptr;
    Shared<EffectComponent>     _effectCom = nullptr;

    bool                        _serverAuthoritative = false; // 서버 권한 모드에서는 로컬 오버랩으로 웨이브를 직접 시작하지 않는다.
    FDelegateHandle             _requiredWaveClearedHandle = {}; // 선행 웨이브 클리어 이벤트를 구독해서 잠긴 2웨이브를 열 때 사용한다.

private:
    string  _waveTag = "GamePlayClearWave"; // 이 트리거가 시작/클리어 시 브로드캐스트하는 웨이브 식별 태그다.
    string  _activateOnWaveClearTag = ""; // 비어있지 않으면 이 태그의 웨이브가 클리어된 뒤에만 현재 트리거가 활성화된다.
    bool    _triggerOnce = false; // true면 한 번만 발동되고 다시는 재사용되지 않는다.
    bool    _hasTriggered = false; // 현재 웨이브가 이미 시작되었는지 추적한다.
    bool    _waveClearBroadcasted = false; // 현재 웨이브 클리어 이벤트를 이미 보냈는지 추적한다.
    bool    _isLockedUntilRequiredWaveClear = false; // 선행 웨이브 클리어 전까지 현재 트리거가 잠겨 있는지 나타낸다.

    float   _clearDelaySec = 1.f; // 몬스터 전멸 후 실제 클리어 판정까지 기다릴 시간이다.
    float   _clearElapsed = 0.f; // 전멸 이후 누적된 클리어 대기 시간이다.

    wstring _spawnLayerTag = TEXT("Layer_Builder"); // 스폰된 몬스터를 추가할 레이어 태그다.

    string  _defaultEffectAssetName = ""; // 웨이브 대기 상태에서 반복 재생할 기본 이펙트 이름이다.
    string  _triggerEffectAssetName = ""; // 웨이브 시작 순간 한 번 재생할 이펙트 이름이다.

    bool    _playDefaultEffectOnBeginPlay = true; // BeginPlay 시 기본 이펙트를 자동 재생할지 결정한다.

    bool    _stopDefaultEffectOnTrigger = true; // 웨이브 시작 시 기본 대기 이펙트를 끌지 결정한다.
    bool    _playTriggerEffectOnTrigger = true; // 웨이브 시작 시 트리거 이펙트를 재생할지 결정한다.
    bool    _showMissionMarker = true; // 활성화된 웨이브 트리거를 미션 마커로 표시할지 결정한다.

    vector<FWaveSpawnEntry>     _spawnEntries; // 이 웨이브에서 소환할 프리팹 목록이다.
    vector<Weak<GameObject>>    _spawnedMonsters; // 실제 스폰 후 생존 추적 중인 몬스터 참조 목록이다.
    vector<FPrewarmedSpawnEntry> _prewarmedMonsters; // 시작 전에 미리 만들어 둔 스폰 후보 목록이다.

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
