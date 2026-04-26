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

    void    Try_TriggerWave(Shared<GameObject> otherObject);
    void    Prewarm_Wave();
    void    Spawn_Wave();
    void    Update_SpawnedMonsters(float timeDelta);
    void    Finish_WaveClear();

    bool    Is_TrackedMonsterDead(const Shared<GameObject>& obj) const;

private:
    Shared<Collider>            _triggerCollider = nullptr;
    Shared<EffectComponent>     _effectCom = nullptr;

     bool _serverAuthoritative = false;

private:
    string  _waveTag = "GamePlayClearWave";
    bool    _triggerOnce = false;
    bool    _hasTriggered = false;
    bool    _waveClearBroadcasted = false; // 웨이브 클리어

    float   _clearDelaySec = 1.f;
    float   _clearElapsed = 0.f;

    wstring _spawnLayerTag = TEXT("Layer_Builder");

    string  _defaultEffectAssetName = "";
    string  _triggerEffectAssetName = "";

    bool    _playDefaultEffectOnBeginPlay = true;

    bool    _stopDefaultEffectOnTrigger = true;
    bool    _playTriggerEffectOnTrigger = true;
    bool    _showMissionMarker = true;

    vector<FWaveSpawnEntry>     _spawnEntries;
    vector<Weak<GameObject>>    _spawnedMonsters;
    vector<FPrewarmedSpawnEntry> _prewarmedMonsters;

public:
    static Shared<GameObject> Create(ComPtr<Device> device, ComPtr<DeviceContext> context);
    Shared<GameObject> Clone(void* arg) override;
    void Free() override;
};

NS_END
