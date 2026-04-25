#include "pch.h"
#include "Skill_ChibakuTensei.h"

#include "AttachedEffectObject.h"
#include "Character.h"
#include "Collider.h"
#include "GameObject_Factory.h"
#include "Transform.h"
#include "Utils.h"

REGISTER_GAMEOBJECT_CATEGORY(Skill_ChibakuTensei, Protocol::OBJECT_TYPE_CHIBAKU_TENSEI, "SkillSpawn");

Skill_ChibakuTensei::Skill_ChibakuTensei(ComPtr<Device> device, ComPtr<DeviceContext> context)
    : SkillObject_Projectile(device, context)
{
}

Skill_ChibakuTensei::Skill_ChibakuTensei(const Skill_ChibakuTensei& rhs)
    : SkillObject_Projectile(rhs)
    , _stoneCount(rhs._stoneCount)
    , _stoneSpawnInterval(rhs._stoneSpawnInterval)
    , _stoneAttachDuration(rhs._stoneAttachDuration)
    , _coreRevealRatio(rhs._coreRevealRatio)
    , _sequenceLifetime(rhs._sequenceLifetime)
    , _attachRadius(rhs._attachRadius)
    , _startRadius(rhs._startRadius)
{
}

HRESULT Skill_ChibakuTensei::Initialize_Prototype()
{
    _lifetime = 4.5f;
    _speed = 24.f;
    _maxDistance = 18.f;
    _damage = 0.f;
    _hitLaunchForce = 0.f;
    _maxHitCount = 1;
    _colliderRadius = 0.65f;
    _collisionPreset = Collision_Preset::Monster_Attack;

    return SkillObject_Projectile::Initialize_Prototype();
}

HRESULT Skill_ChibakuTensei::Initialize(void* arg)
{
    CHECK_FAILED(SkillObject_Projectile::Initialize(arg), E_FAIL);

    _attachTarget.reset();
    _stones.clear();
    _coreShell.reset();
    _attachSequenceStarted = false;
    _coreShellSpawned = false;
    _sequenceElapsed = 0.f;
    _nextStoneIndex = 0;

    return S_OK;
}

void Skill_ChibakuTensei::Update(float timeDelta)
{
    SkillObject_Projectile::Update(timeDelta);

    if (Is_Destroy())
        return;

    if (!_attachSequenceStarted)
        return;

    _sequenceElapsed += timeDelta;

    Update_TargetHold(timeDelta);

    Spawn_ReadyStones();
    Update_AttachedStones(timeDelta);

    const float revealIndex = static_cast<float>(_stoneCount) * _coreRevealRatio;
    if (!_coreShellSpawned && static_cast<float>(_nextStoneIndex) >= revealIndex)
        Spawn_CoreShell();

    if (_sequenceElapsed >= _sequenceLifetime)
        Set_Destroy(true);
}

void Skill_ChibakuTensei::OnBeginOverlap(Shared<Collider> self, Shared<Collider> other)
{
    SkillObject::OnBeginOverlap(self, other);

    if (_attachSequenceStarted || Is_Destroy())
        return;

    Character* target = Find_HitCharacter(other);
    if (!target)
        return;

    Begin_AttachSequence(target);
}

Character* Skill_ChibakuTensei::Find_HitCharacter(Shared<Collider> other) const
{
    if (!other)
        return nullptr;

    auto otherOwner = other->Get_Owner();
    auto owner = _owner.lock();
    if (!otherOwner || otherOwner == owner)
        return nullptr;

    return dynamic_cast<Character*>(otherOwner.get());
}

void Skill_ChibakuTensei::Begin_AttachSequence(Character* target)
{
    CHECK_NULL(target);

    _attachTarget = dynamic_pointer_cast<Character>(target->GetSharedPtr<GameObject>());
    _attachSequenceStarted = true;
    _coreShellSpawned = false;
    _sequenceElapsed = 0.f;
    _nextStoneIndex = 0;
    _stones.clear();

    _isMoving = false;

    if (_collider)
        _collider->Set_IsActive(false);

    Vec3 targetCenter = Vec3::Zero;
    if (Try_GetTargetCenter(targetCenter) && _transformCom)
        _transformCom->Set_WorldPosition(targetCenter);
}

void Skill_ChibakuTensei::Launch(const Vec3& direction)
{
    SkillObject_Projectile::Launch(direction);
    Set_Destroy(true);
}

void Skill_ChibakuTensei::Spawn_ReadyStones()
{
    if (_nextStoneIndex >= _stoneCount)
        return;

    Vec3 targetCenter = Vec3::Zero;
    if (!Try_GetTargetCenter(targetCenter))
    {
        Set_Destroy(true);
        return;
    }

    static const array<string, 4> kStoneEffectNames =
    {
        "Stone_01",
        "Stone_02",
        "Stone_03",
        "Stone_04",
    };

    while (_nextStoneIndex < _stoneCount)
    {
        const float spawnTime = static_cast<float>(_nextStoneIndex) * _stoneSpawnInterval;
        if (_sequenceElapsed < spawnTime)
            break;

        FAttachedStone stone{};
        stone.spawnTime = spawnTime;
        stone.attachDuration = _stoneAttachDuration + static_cast<float>(_nextStoneIndex % 3) * 0.035f;
        stone.localAttachOffset = Build_AttachOffset(_nextStoneIndex);
        stone.startWorldPosition = targetCenter + Build_StartOffset(_nextStoneIndex);
        stone.localRotation = Build_StoneRotation(_nextStoneIndex);
        stone.localScale = Build_StoneScale(_nextStoneIndex);

        const string& effectName = kStoneEffectNames[_nextStoneIndex % static_cast<int32>(kStoneEffectNames.size())];
        stone.effectObject = Spawn_AttachedEffect(effectName, stone.startWorldPosition, stone.localRotation, stone.localScale);

        _stones.push_back(stone);
        ++_nextStoneIndex;
    }
}

void Skill_ChibakuTensei::Update_AttachedStones(float timeDelta)
{
    Vec3 targetCenter = Vec3::Zero;
    if (!Try_GetTargetCenter(targetCenter))
    {
        Set_Destroy(true);
        return;
    }

    for (FAttachedStone& stone : _stones)
    {
        auto effect = stone.effectObject.lock();
        if (!effect || effect->Is_Destroy())
            continue;

        const float progress = ::clamp((_sequenceElapsed - stone.spawnTime) / stone.attachDuration, 0.f, 1.f);
        const float easedProgress = progress * progress * (3.f - 2.f * progress);
        const Vec3 targetPosition = targetCenter + stone.localAttachOffset;
        const Vec3 currentPosition = stone.startWorldPosition + (targetPosition - stone.startWorldPosition) * easedProgress;

        auto transform = effect->Get_Transform();
        if (!transform)
            continue;

        transform->Set_WorldPosition(currentPosition);
        transform->Set_WorldRotation(stone.localRotation.x, stone.localRotation.y, stone.localRotation.z);
        transform->Set_LocalScale(stone.localScale);
    }

    auto core = _coreShell.lock();
    if (core && !core->Is_Destroy())
    {
        if (auto transform = core->Get_Transform())
        {
            transform->Set_WorldPosition(targetCenter);
            transform->Rotate_Axis(Vec3::Up, timeDelta * 18.f);
        }
    }
}

void Skill_ChibakuTensei::Spawn_CoreShell()
{
    Vec3 targetCenter = Vec3::Zero;
    if (!Try_GetTargetCenter(targetCenter))
        return;

    _coreShell = Spawn_AttachedEffect(
        "ChibakuTensei_Core",
        targetCenter,
        Vec3(0.f, 0.f, 0.f),
        Vec3(1.08f, 1.08f, 1.08f));

    _coreShellSpawned = true;
}

Shared<GameObject> Skill_ChibakuTensei::Spawn_AttachedEffect(const string& effectAssetName, const Vec3& worldPosition, const Vec3& worldRotation, const Vec3& worldScale)
{
    AttachedEffectObject::FAttachedEffectObjectDesc desc{};
    desc.effectAssetName = effectAssetName;
    desc.loopOverride = true;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        Protocol::OBJECT_TYPE_ATTACHED_EFFECT,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    if (!spawned)
        return nullptr;

    if (auto transform = spawned->Get_Transform())
    {
        transform->Set_WorldPosition(worldPosition);
        transform->Set_WorldRotation(worldRotation.x, worldRotation.y, worldRotation.z);
        transform->Set_LocalScale(worldScale);
    }

    return spawned;
}

bool Skill_ChibakuTensei::Try_GetTargetCenter(Vec3& outCenter) const
{
    auto target = _attachTarget.lock();
    if (!target || target->Is_Destroy())
        return false;

    auto targetTransform = target->Get_Transform();
    if (!targetTransform)
        return false;

    outCenter = targetTransform->Get_WorldPosition() + Vec3(0.f, 1.05f, 0.f);
    return true;
}

void Skill_ChibakuTensei::Update_TargetHold(float timeDelta)
{
    if (_targetHoldFinished)
        return;

    auto target = _attachTarget.lock();
    if (!target || target->Is_Destroy())
        return;

    auto targetTransform = target->Get_Transform();
    if (!targetTransform)
        return;

    if (_sequenceElapsed < _targetHoldDelay)
        return;

    auto targetMovement = target->Get_Component<MovementComponent>();

    if (!_targetHoldStarted)
    {
        _targetHoldStarted = true;
        _targetHoldElapsed = 0.f;

        _targetHoldPosition = targetTransform->Get_WorldPosition();

        if (targetMovement)
            targetMovement->Set_Velocity(Vec3::Zero);
    }

    if (_targetHoldElapsed < _targetHoldDuration)
    {
        targetTransform->Set_WorldPosition(_targetHoldPosition);

        if (targetMovement)
            targetMovement->Set_Velocity(Vec3::Zero);

        _targetHoldElapsed += timeDelta;
        return;
    }

    _targetHoldFinished = true;
}

Vec3 Skill_ChibakuTensei::Build_AttachOffset(int32 stoneIndex) const
{
    static const array<Vec3, 28> kAttachDirections =
    {
        Vec3(0.00f, 1.00f, 0.05f), Vec3(0.32f, 0.88f, 0.35f), Vec3(-0.38f, 0.82f, 0.42f), Vec3(0.58f, 0.64f, -0.28f),
        Vec3(-0.60f, 0.60f, -0.32f), Vec3(0.18f, 0.48f, 0.86f), Vec3(-0.22f, 0.42f, -0.88f), Vec3(0.86f, 0.28f, 0.38f),
        Vec3(-0.82f, 0.24f, 0.46f), Vec3(0.72f, 0.12f, -0.68f), Vec3(-0.70f, 0.10f, -0.70f), Vec3(0.22f, -0.05f, 0.98f),
        Vec3(-0.28f, -0.10f, -0.96f), Vec3(0.98f, -0.14f, 0.08f), Vec3(-0.98f, -0.16f, 0.12f), Vec3(0.44f, -0.42f, 0.80f),
        Vec3(-0.50f, -0.46f, 0.74f), Vec3(0.52f, -0.50f, -0.70f), Vec3(-0.48f, -0.54f, -0.70f), Vec3(0.02f, -0.84f, 0.54f),
        Vec3(0.06f, -0.88f, -0.48f), Vec3(0.66f, -0.72f, 0.20f), Vec3(-0.64f, -0.74f, 0.20f), Vec3(0.34f, 0.70f, 0.62f),
        Vec3(-0.36f, 0.68f, -0.64f), Vec3(0.76f, 0.48f, 0.44f), Vec3(-0.78f, 0.46f, -0.42f), Vec3(0.00f, -1.00f, 0.04f),
    };

    Vec3 dir = kAttachDirections[stoneIndex % static_cast<int32>(kAttachDirections.size())];
    dir = Utils::Safe_Normalize(dir, Vec3::Up);

    const float radiusBias = 0.82f + static_cast<float>(stoneIndex % 4) * 0.08f;
    return dir * (_attachRadius * radiusBias);
}

Vec3 Skill_ChibakuTensei::Build_StartOffset(int32 stoneIndex) const
{
    Vec3 dir = Build_AttachOffset(stoneIndex);
    dir = Utils::Safe_Normalize(dir, Vec3::Forward);

    const Vec3 swirl = Vec3(dir.z, 0.25f + static_cast<float>(stoneIndex % 5) * 0.08f, -dir.x);
    const Vec3 startDir = Utils::Safe_Normalize(dir + swirl * 0.55f, dir);
    const float radius = _startRadius + static_cast<float>(stoneIndex % 4) * 0.35f;

    return startDir * radius;
}

Vec3 Skill_ChibakuTensei::Build_StoneRotation(int32 stoneIndex) const
{
    return Vec3(
        static_cast<float>((stoneIndex * 37) % 360),
        static_cast<float>((stoneIndex * 71) % 360),
        static_cast<float>((stoneIndex * 113) % 360));
}

Vec3 Skill_ChibakuTensei::Build_StoneScale(int32 stoneIndex) const
{
    static const array<float, 6> kScalePattern =
    {
        0.62f, 0.78f, 0.92f, 0.70f, 1.05f, 0.84f,
    };

    const float scale = kScalePattern[stoneIndex % static_cast<int32>(kScalePattern.size())];
    return Vec3(scale, scale, scale);
}

Shared<GameObject> Skill_ChibakuTensei::Create(ComPtr<Device> device, ComPtr<DeviceContext> context)
{
    auto instance = make_shared<Skill_ChibakuTensei>(device, context);

    if (FAILED(instance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Create : Skill_ChibakuTensei");
        return nullptr;
    }

    return instance;
}

Shared<GameObject> Skill_ChibakuTensei::Clone(void* arg)
{
    auto clone = make_shared<Skill_ChibakuTensei>(*this);

    if (FAILED(clone->Initialize(arg)))
    {
        MSG_BOX("Failed to Clone : Skill_ChibakuTensei");
        return nullptr;
    }

    return clone;
}

void Skill_ChibakuTensei::Free()
{
    for (FAttachedStone& stone : _stones)
    {
        auto effect = stone.effectObject.lock();
        if (effect && !effect->Is_Destroy())
            effect->Set_Destroy(true);
    }

    auto core = _coreShell.lock();
    if (core && !core->Is_Destroy())
        core->Set_Destroy(true);

    SkillObject_Projectile::Free();
}
