#include "pch.h"
#include "WaveTrigger.h"

#include "GameObject_Factory.h"
#include "Bounding_OBB.h"
#include "Collider.h"
#include "EffectComponent.h"
#include "MyPlayer.h"
#include "CombatStat.h"
#include "Debug_Manager.h"

#include "NetworkManager.h"

REGISTER_GAMEOBJECT(WaveTrigger, Protocol::OBJECT_TYPE_WAVE_TRIGGER)
IMPLEMENT_REFLECTION(WaveTrigger)

static Vec3 Resolve_WaveTriggerWorldPosition(Shared<Transform> triggerTransform, const Vec3& localPosition)
{
    if (!triggerTransform)
        return localPosition;

    return Vec3::Transform(localPosition, triggerTransform->Get_WorldMatrix());
}

static Vec3 Resolve_WaveTriggerWorldRotation(Shared<Transform> triggerTransform, const Vec3& localRotationDegree)
{
    if (!triggerTransform)
        return localRotationDegree;

    const Quat triggerWorldRotation = triggerTransform->Get_WorldRotation();
    const Quat localRotation = Quat::CreateFromYawPitchRoll(
        XMConvertToRadians(localRotationDegree.y),
        XMConvertToRadians(localRotationDegree.x),
        XMConvertToRadians(localRotationDegree.z));

    Quat finalRotation = triggerWorldRotation * localRotation;
    finalRotation.Normalize();

    const Vec3 finalEulerRadian = finalRotation.ToEuler();

    return Vec3(
        XMConvertToDegrees(finalEulerRadian.x),
        XMConvertToDegrees(finalEulerRadian.y),
        XMConvertToDegrees(finalEulerRadian.z));
}

static Vec3 Resolve_WaveTriggerWorldScale(Shared<Transform> triggerTransform, const Vec3& localScale)
{
    Vec3 safeScale = localScale;
    if (safeScale.LengthSquared() <= FLT_EPSILON)
        safeScale = Vec3::One;

    if (!triggerTransform)
        return safeScale;

    const Vec3 triggerScale = triggerTransform->Get_WorldScale();

    return Vec3(
        safeScale.x * triggerScale.x,
        safeScale.y * triggerScale.y,
        safeScale.z * triggerScale.z);
}

static Vec3 Json_ToVec3(const json& value, const Vec3& fallback = Vec3::Zero)
{
    if (!value.is_array() || value.size() < 3)
        return fallback;

    return Vec3(
        value[0].get<float>(),
        value[1].get<float>(),
        value[2].get<float>());
}

static json Vec3_ToJson(const Vec3& value)
{
    return json::array({ value.x, value.y, value.z });
}

bool WaveTrigger::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "WaveTrigger";
    info.properties.clear();

    PROPERTY_STRING_JSON("웨이브 태그", "wave_tag", _waveTag);
    PROPERTY_BOOL_JSON("1회 트리거", "trigger_once", _triggerOnce);
    PROPERTY_FLOAT_JSON("클리어 지연", "clear_delay_sec", _clearDelaySec, 0.f, 30.f);

    PROPERTY_STRING_JSON("기본 이펙트", "default_effect_asset_name", _defaultEffectAssetName);
    PROPERTY_STRING_JSON("트리거 이펙트", "trigger_effect_asset_name", _triggerEffectAssetName);
    PROPERTY_BOOL_JSON("BeginPlay 기본 이펙트", "play_default_effect_on_begin_play", _playDefaultEffectOnBeginPlay);
    PROPERTY_BOOL_JSON("트리거 시 기본 이펙트 중지", "stop_default_effect_on_trigger", _stopDefaultEffectOnTrigger);
    PROPERTY_BOOL_JSON("트리거 시 트리거 이펙트 재생", "play_trigger_effect_on_trigger", _playTriggerEffectOnTrigger);
    PROPERTY_BOOL_JSON("미션 마커 표시", "show_mission_marker", _showMissionMarker);

    return true;
}

WaveTrigger::WaveTrigger(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : GameObject(device, context)
{
}

WaveTrigger::WaveTrigger(const WaveTrigger& rhs)
    : GameObject(rhs)
    , _waveTag(rhs._waveTag)
    , _triggerOnce(rhs._triggerOnce)
    , _hasTriggered(false)
    , _waveClearBroadcasted(false)
    , _clearDelaySec(rhs._clearDelaySec)
    , _clearElapsed(0.f)
    , _spawnLayerTag(rhs._spawnLayerTag)
    , _defaultEffectAssetName(rhs._defaultEffectAssetName)
    , _triggerEffectAssetName(rhs._triggerEffectAssetName)
    , _playDefaultEffectOnBeginPlay(rhs._playDefaultEffectOnBeginPlay)
    , _stopDefaultEffectOnTrigger(rhs._stopDefaultEffectOnTrigger)
    , _playTriggerEffectOnTrigger(rhs._playTriggerEffectOnTrigger)
    , _showMissionMarker(rhs._showMissionMarker)
    , _spawnEntries(rhs._spawnEntries)
{
}

HRESULT WaveTrigger::Initialize_Prototype()
{
    return GameObject::Initialize_Prototype();
}

HRESULT WaveTrigger::Initialize(void* arg)
{
    CHECK_FAILED(GameObject::Initialize(arg), E_FAIL);
    CHECK_FAILED(Ready_Components(), E_FAIL);

    _hasTriggered = false;
    _waveClearBroadcasted = false;
    _clearElapsed = 0.f;
    _spawnedMonsters.clear();
    _prewarmedMonsters.clear();

    if (_triggerCollider)
    {
        _triggerCollider->Set_CollisionPreset(Collision_Preset::Trigger);
        _triggerCollider->Set_IsActive(true);
    }

    return S_OK;
}

void WaveTrigger::BeginPlay()
{
    GameObject::BeginPlay();

    if (_showMissionMarker)
        GAME->Get_DelegateHub().Set_MissionMarkerTarget(GetSharedPtr<GameObject>());

    if (!_serverAuthoritative)
        Prewarm_Wave();

    if (!_effectCom)
        return;

    if (!_playDefaultEffectOnBeginPlay)
        return;

    if (_defaultEffectAssetName.empty())
        return;

    EffectComponent::FPlayDesc playDesc{};
    playDesc.effectAssetName = _defaultEffectAssetName;
    playDesc.loopOverride = true;

    _effectCom->Play_Effect(playDesc);
}

void WaveTrigger::Refresh_ForEditorPlay()
{
    _hasTriggered = false;
    _waveClearBroadcasted = false;
    _clearElapsed = 0.f;
    _spawnedMonsters.clear();
    _prewarmedMonsters.clear();

    if (_triggerCollider)
        _triggerCollider->Set_IsActive(true);

    if (_serverAuthoritative)
        return;

    Prewarm_Wave();
}

void WaveTrigger::Set_ServerAuthoritative(bool enabled)
{
    _serverAuthoritative = enabled;

    if (!_serverAuthoritative)
        return;

    _hasTriggered = false;
    _waveClearBroadcasted = false;
    _clearElapsed = 0.f;
    _spawnedMonsters.clear();
    _prewarmedMonsters.clear();

    if (_triggerCollider)
        _triggerCollider->Set_IsActive(true);
}

void WaveTrigger::Update(float timeDelta)
{
    GameObject::Update(timeDelta);

    if (_effectCom)
        _effectCom->Update(timeDelta);

    if (!_hasTriggered)
        return;

    Update_SpawnedMonsters(timeDelta);
}

void WaveTrigger::Prewarm_Wave()
{
    _prewarmedMonsters.clear();
    _prewarmedMonsters.reserve(_spawnEntries.size());

    for (const auto& entry : _spawnEntries)
    {
        if (entry.prefabName.empty())
            continue;

        json overrides = json::object();
        if (entry.objectTypeOverride != 0)
            overrides["object_type"] = entry.objectTypeOverride;

        auto prewarmedMonster = GAME->Instantiate_Prefab(entry.prefabName, overrides);
        if (!prewarmedMonster)
        {
            LOG_WARN("[WaveTrigger] Failed to prewarm prefab. prefabName={}", entry.prefabName);
            continue;
        }

        FPrewarmedSpawnEntry prewarmedEntry{};
        prewarmedEntry.monster = prewarmedMonster;
        prewarmedEntry.source = entry;
        prewarmedEntry.isSpawned = false;

        _prewarmedMonsters.emplace_back(std::move(prewarmedEntry));
    }
}

void WaveTrigger::Late_Update(float timeDelta)
{
    GameObject::Late_Update(timeDelta);

    if (_effectCom)
        _effectCom->Late_Update(timeDelta);

    if (!Is_Destroy() && _triggerCollider && _transformCom)
    {
        _triggerCollider->Update_Collider(_transformCom->Get_WorldMatrix());
        GAME->Add_Collider(_triggerCollider);
    }

    if (_triggerCollider->Get_IsActive())
    {
        for (const auto& entry : _spawnEntries)
        {
            FDebugSphereDesc debugSphereDesc{};
            debugSphereDesc.center = Resolve_WaveTriggerWorldPosition(_transformCom, entry.position);
            debugSphereDesc.radius = 0.35f;
            debugSphereDesc.style.color = Color(0.2f, 0.8f, 1.f, 1.f);
            debugSphereDesc.style.duration = 0.f;
            debugSphereDesc.style.depthEnabled = true;

            GAME->Draw_DebugSphere(debugSphereDesc);
        }
    }

}

HRESULT WaveTrigger::Render()
{
    return GameObject::Render();
}

json WaveTrigger::To_Json() const
{
    json root = GameObject::To_Json();
    json custom = json::object();

    custom["wave_tag"] = _waveTag;
    custom["trigger_once"] = _triggerOnce;
    custom["clear_delay_sec"] = _clearDelaySec;
    custom["spawn_layer_tag"] = Utils::ToString(_spawnLayerTag);

    custom["default_effect_asset_name"] = _defaultEffectAssetName;
    custom["trigger_effect_asset_name"] = _triggerEffectAssetName;
    custom["play_default_effect_on_begin_play"] = _playDefaultEffectOnBeginPlay;
    custom["stop_default_effect_on_trigger"] = _stopDefaultEffectOnTrigger;
    custom["play_trigger_effect_on_trigger"] = _playTriggerEffectOnTrigger;
    custom["show_mission_marker"] = _showMissionMarker;

    json spawnEntriesJson = json::array();

    for (const auto& entry : _spawnEntries)
    {
        json entryJson;
        entryJson["prefab_name"] = entry.prefabName;
        entryJson["position"] = Vec3_ToJson(entry.position);
        entryJson["rotation"] = Vec3_ToJson(entry.rotation);
        entryJson["scale"] = Vec3_ToJson(entry.scale);

        const auto objectType = static_cast<Protocol::OBJECT_TYPE>(entry.objectTypeOverride);
        const auto objectTypeName = magic_enum::enum_name(objectType);

        if (!objectTypeName.empty())
            entryJson["object_type_override"] = string(objectTypeName);
        else
            entryJson["object_type_override"] = entry.objectTypeOverride;

        spawnEntriesJson.emplace_back(std::move(entryJson));
    }

    custom["spawn_entries"] = std::move(spawnEntriesJson);
    root["custom_properties"] = std::move(custom);

    return root;
}

void WaveTrigger::From_Json(const json& data)
{
    GameObject::From_Json(data);

    // 이건 어쩔수없이 다시 하드코딩
    if (data.contains("wave_tag"))
        _waveTag = data["wave_tag"].get<string>();

    if (data.contains("trigger_once"))
        _triggerOnce = data["trigger_once"].get<bool>();

    if (data.contains("clear_delay_sec"))
        _clearDelaySec = data["clear_delay_sec"].get<float>();

    if (data.contains("spawn_layer_tag"))
        _spawnLayerTag = Utils::ToWString(data["spawn_layer_tag"].get<string>());

    if (data.contains("default_effect_asset_name"))
        _defaultEffectAssetName = data["default_effect_asset_name"].get<string>();

    if (data.contains("trigger_effect_asset_name"))
        _triggerEffectAssetName = data["trigger_effect_asset_name"].get<string>();

    if (data.contains("play_default_effect_on_begin_play"))
        _playDefaultEffectOnBeginPlay = data["play_default_effect_on_begin_play"].get<bool>();

    if (data.contains("stop_default_effect_on_trigger"))
        _stopDefaultEffectOnTrigger = data["stop_default_effect_on_trigger"].get<bool>();

    if (data.contains("play_trigger_effect_on_trigger"))
        _playTriggerEffectOnTrigger = data["play_trigger_effect_on_trigger"].get<bool>();

    if (data.contains("show_mission_marker"))
        _showMissionMarker = data["show_mission_marker"].get<bool>();

    _spawnEntries.clear();

    if (!data.contains("spawn_entries") || !data["spawn_entries"].is_array())
        return;

    for (const auto& entryJson : data["spawn_entries"])
    {
        FWaveSpawnEntry entry{};

        if (entryJson.contains("prefab_name"))
            entry.prefabName = entryJson["prefab_name"].get<string>();

        if (entryJson.contains("position"))
            entry.position = Json_ToVec3(entryJson["position"], Vec3::Zero);

        if (entryJson.contains("rotation"))
            entry.rotation = Json_ToVec3(entryJson["rotation"], Vec3::Zero);

        if (entryJson.contains("scale"))
            entry.scale = Json_ToVec3(entryJson["scale"], Vec3::One);

        if (entryJson.contains("object_type_override"))
        {
            if (entryJson["object_type_override"].is_string())
            {
                const auto objectType = magic_enum::enum_cast<Protocol::OBJECT_TYPE>(
                    entryJson["object_type_override"].get<string>());

                if (objectType.has_value())
                    entry.objectTypeOverride = static_cast<uint32>(objectType.value());
            }
            else if (entryJson["object_type_override"].is_number_integer())
            {
                entry.objectTypeOverride = entryJson["object_type_override"].get<uint32>();
            }
        }

        if (entry.scale.LengthSquared() <= FLT_EPSILON)
            entry.scale = Vec3::One;

        _spawnEntries.emplace_back(std::move(entry));
    }
}

void WaveTrigger::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    GameObject::OnBeginOverlap(self, other);

    if (!_triggerCollider || self != _triggerCollider)
        return;

    if (!other)
        return;

    auto otherObject = other->Get_Owner();
    if (!otherObject || otherObject.get() == this)
        return;

    Try_TriggerWave(otherObject);
}

HRESULT WaveTrigger::Ready_Components()
{
    {
        Bounding_OBB::FBoundingOBBDesc triggerDesc{};
        triggerDesc.extents = Vec3(2.f, 2.f, 2.f);
        triggerDesc.radians = Vec3::Zero;

        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_COLLIDER_OBB, _triggerCollider, &triggerDesc), E_FAIL);
        CHECK_NULL(_triggerCollider, E_FAIL);

        _triggerCollider->Set_CollisionPreset(Collision_Preset::Trigger);
        _triggerCollider->Set_IsActive(true);
    }

    {
        CHECK_FAILED(Add_Component(Protocol::COMPONENT_TYPE_EFFECT, _effectCom), E_FAIL);
    }

    return S_OK;
}

void WaveTrigger::Try_TriggerWave(Shared<GameObject> otherObject)
{
    if (_serverAuthoritative)
        return;

    if (!otherObject)
        return;

    auto myPlayer = dynamic_pointer_cast<MyPlayer>(otherObject);
    if (!myPlayer)
        return;

    if (_hasTriggered)
        return;

    _hasTriggered = true;
    _waveClearBroadcasted = false;
    _clearElapsed = 0.f;
    _spawnedMonsters.clear();

    if (_triggerOnce && _triggerCollider)
        _triggerCollider->Set_IsActive(false);

    if (_effectCom)
    {
        if (_stopDefaultEffectOnTrigger)
            _effectCom->Stop_Effect();

        if (_playTriggerEffectOnTrigger && !_triggerEffectAssetName.empty())
        {
            EffectComponent::FPlayDesc playDesc{};
            playDesc.effectAssetName = _triggerEffectAssetName;
            playDesc.loopOverride = false;

            _effectCom->Play_Effect(playDesc);
        }
    }

    if (_showMissionMarker)
        GAME->Get_DelegateHub().Clear_MissionMarkerTarget();

    GAME->Get_DelegateHub().OnWaveStarted.Broadcast(_waveTag);

    Spawn_Wave();
}

void WaveTrigger::Spawn_Wave()
{
    if (_prewarmedMonsters.empty())
    {
        LOG_WARN("[WaveTrigger] No prewarmed monsters. waveTag={}", _waveTag);
        Finish_WaveClear();
        return;
    }

    for (auto& prewarmedEntry : _prewarmedMonsters)
    {
        if (!prewarmedEntry.monster || prewarmedEntry.isSpawned)
            continue;

        const auto& entry = prewarmedEntry.source;
        const Vec3 spawnPosition = Resolve_WaveTriggerWorldPosition(_transformCom, entry.position);
        const Vec3 spawnRotation = Resolve_WaveTriggerWorldRotation(_transformCom, entry.rotation);
        const Vec3 spawnScale = Resolve_WaveTriggerWorldScale(_transformCom, entry.scale);

        auto transform = prewarmedEntry.monster->Get_Transform();
        if (transform)
        {
            transform->Set_WorldPosition(spawnPosition);
            transform->Set_LocalRotation(
                spawnRotation.x,
                spawnRotation.y,
                spawnRotation.z);
            transform->Set_LocalScale(spawnScale);
        }

        if (FAILED(GAME->Add_GameObject(Get_LevelIndex(), _spawnLayerTag, prewarmedEntry.monster)))
        {
            LOG_WARN("[WaveTrigger] Failed to activate prewarmed prefab. prefabName={}", entry.prefabName);
            continue;
        }

        prewarmedEntry.isSpawned = true;

        auto combatStat = prewarmedEntry.monster->Get_Component<CombatStat>();
        if (!combatStat)
        {
            LOG_WARN("[WaveTrigger] Spawned object has no CombatStat. prefabName={}", entry.prefabName);
            continue;
        }

        _spawnedMonsters.emplace_back(prewarmedEntry.monster);
    }

    if (_spawnedMonsters.empty())
        Finish_WaveClear();
}

void WaveTrigger::Update_SpawnedMonsters(float timeDelta)
{
    if (_waveClearBroadcasted)
        return;

    vector<Weak<GameObject>> aliveMonsters;
    aliveMonsters.reserve(_spawnedMonsters.size());

    for (auto& weakObject : _spawnedMonsters)
    {
        auto obj = weakObject.lock();
        if (!obj)
            continue;

        if (Is_TrackedMonsterDead(obj))
            continue;

        aliveMonsters.emplace_back(obj);
    }

    _spawnedMonsters = std::move(aliveMonsters);

    if (!_spawnedMonsters.empty())
    {
        _clearElapsed = 0.f;
        return;
    }

    _clearElapsed += timeDelta;
    if (_clearElapsed < _clearDelaySec)
        return;

    Finish_WaveClear();
}

void WaveTrigger::Finish_WaveClear()
{
    if (_waveClearBroadcasted)
        return;

    _waveClearBroadcasted = true;
    _clearElapsed = 0.f;
    _spawnedMonsters.clear();
    _prewarmedMonsters.clear();

    LOG_INFO("[WaveTrigger] Wave Clear : {}", _waveTag);

    GAME->Get_DelegateHub().OnWaveCleared.Broadcast(_waveTag);

    if (_triggerOnce)
        return;

    _hasTriggered = false;
    Prewarm_Wave();

    if (_triggerCollider)
        _triggerCollider->Set_IsActive(true);

    if (_effectCom && _playDefaultEffectOnBeginPlay && !_defaultEffectAssetName.empty())
    {
        EffectComponent::FPlayDesc playDesc{};
        playDesc.effectAssetName = _defaultEffectAssetName;
        playDesc.loopOverride = true;

        _effectCom->Play_Effect(playDesc);
    }
}

bool WaveTrigger::Is_TrackedMonsterDead(const Shared<GameObject>& obj) const
{
    if (!obj)
        return true;

    if (obj->Is_Destroy())
        return true;

    auto combatStat = obj->Get_Component<CombatStat>();
    if (!combatStat)
        return obj->Is_Destroy();

    return combatStat->Is_Dead();
}

Shared<GameObject> WaveTrigger::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<WaveTrigger>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : WaveTrigger");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> WaveTrigger::Clone(void* arg)
{
    auto clone = make_shared<WaveTrigger>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Cloned : WaveTrigger");
        return nullptr;
    }

    return clone;
}

void WaveTrigger::Free()
{
    GameObject::Free();
}
