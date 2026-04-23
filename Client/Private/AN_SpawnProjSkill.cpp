#include "pch.h"
#include "AN_SpawnProjSkill.h"

#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject_Projectile.h"
#include "Client_Defines.h"
#include "MyPlayer.h"
#include "TargetComponent.h"

REGISTER_ANIM_NOTIFY(AN_SpawnProjSkill)
IMPLEMENT_REFLECTION(AN_SpawnProjSkill)

bool AN_SpawnProjSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnProjSkill";
    info.properties.clear();

    PROPERTY_ENUM_JSON("스킬 타입", "spawn_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_BOOL_JSON("Forward 사용", "use_owner_forward", _useOwnerForward);
    PROPERTY_BOOL_JSON("타겟을 향해 던질지", "aim_at_target", _aimAtTarget);
    PROPERTY_BOOL_JSON("Y축 무시", "ignore_y", _ignoreY);

    return true;
}

string AN_SpawnProjSkill::Get_TypeName() const
{
    return "AN_SpawnProjSkill";
}

void AN_SpawnProjSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    if (!GameObject_Factory::Is_ObjectInCategory(_spawnObjectType, "SkillSpawn"))
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    if (!ownerTransform)
        return;

    const Vec3 spawnPosition = Calculate_WorldSpawnPosition(ownerTransform, _localOffset);

    Vec3 launchDirection = _useOwnerForward
        ? Resolve_OwnerForward(ownerTransform, _ignoreY)
        : (_ignoreY ? Vec3(0.f, 0.f, 1.f) : Vec3::Forward);

    if (_aimAtTarget)
    {
        launchDirection = Resolve_TargetDirection(
            context.owner,
            spawnPosition,
            _ignoreY,
            launchDirection);
    }

    SkillObject_Projectile::FProjectileSkillDesc desc{};
    desc.ownerObject = context.owner->GetSharedPtr<GameObject>();
    desc.collisionPreset = _collisionPreset;
    desc.spawnPosition = spawnPosition;
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;
    desc.direction = launchDirection;
    desc.startAttached = false;
    desc.useGravity = false;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    if (!spawned)
        return;

    spawned->Set_Owner(context.owner->GetSharedPtr<GameObject>());

    auto projectile = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    if (!projectile)
        return;

    auto projectileTransform = projectile->Get_Transform();
    if (projectileTransform)
    {
        projectileTransform->LookAt(projectileTransform->Get_WorldPosition() + launchDirection);
    }

    projectile->Launch(launchDirection);
}

Vec3 AN_SpawnProjSkill::Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset)
{
    const Vec3 worldPos = transform->Get_WorldPosition();
    const Vec3 right = transform->Get_WorldRight();
    const Vec3 up = transform->Get_WorldUp();
    const Vec3 forward = transform->Get_WorldForward();

    return worldPos
        + right * localOffset.x
        + up * localOffset.y
        + forward * localOffset.z;
}

Vec3 AN_SpawnProjSkill::Resolve_OwnerForward(Shared<Transform> ownerTransform, bool ignoreY)
{
    if (!ownerTransform)
        return Vec3::Forward;

    Vec3 forward = ownerTransform->Get_WorldForward();

    if (ignoreY)
        forward.y = 0.f;

    return Utils::Safe_Normalize(forward, Vec3::Forward);
}

Vec3 AN_SpawnProjSkill::Resolve_TargetDirection(
    GameObject* owner,
    const Vec3& spawnPosition,
    bool ignoreY,
    const Vec3& fallbackDirection)
{
    if (!owner)
        return fallbackDirection;

    auto myPlayer = dynamic_cast<MyPlayer*>(owner);
    if (!myPlayer)
        return fallbackDirection;

    auto targetCom = myPlayer->Get_Component<TargetComponent>();
    if (!targetCom || !targetCom->IsLockOn())
        return fallbackDirection;

    auto lockedTarget = targetCom->Get_LockedTarget().lock();
    if (!lockedTarget)
        return fallbackDirection;

    auto targetTransform = lockedTarget->Get_Transform();
    if (!targetTransform)
        return fallbackDirection;

    Vec3 targetPosition = targetTransform->Get_WorldPosition();
    targetPosition.y += 0.5f;

    Vec3 targetDirection = targetPosition - spawnPosition;

    if (ignoreY)
        targetDirection.y = 0.f;

    return Utils::Safe_Normalize(targetDirection, fallbackDirection);
}
