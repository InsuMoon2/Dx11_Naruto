#include "pch.h"
#include "AN_SpawnSkill.h"

#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject.h"
#include "SkillObject_Projectile.h"
#include "Client_Defines.h"
#include "MyPlayer.h"
#include "TargetComponent.h"

REGISTER_ANIM_NOTIFY(AN_SpawnSkill)

IMPLEMENT_REFLECTION(AN_SpawnSkill)

bool AN_SpawnSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_SpawnSkill";
    info.properties.clear();

    PROPERTY_ENUM_JSON("스킬 타입", "spawn_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);
    PROPERTY_VEC3_JSON("로컬 오프셋", "local_offset", _localOffset, 0.1f);
    PROPERTY_BOOL_JSON("Forward 사용", "use_owner_forward", _useOwnerForward);
    PROPERTY_BOOL_JSON("타겟을 향해 던질지", "aim_at_target", _aimAtTarget);
    PROPERTY_BOOL_JSON("락온 타겟 위치에 스폰", "spawn_at_locked_target", _spawnAtLockedTarget);
    PROPERTY_VEC3_JSON("타겟 기준 오프셋", "target_offset", _targetOffset, 0.1f);
    PROPERTY_BOOL_JSON("Projectile이면 즉시 발사", "launch_if_projectile", _launchIfProjectile);

    return true;
}

string AN_SpawnSkill::Get_TypeName() const
{
    return "AN_SpawnSkill";
}

void AN_SpawnSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    if (!context.owner)
        return;

    if (!GameObject_Factory::Is_ObjectInCategory(_spawnObjectType, "SkillSpawn"))
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    if (!ownerTransform)
        return;

    Vec3 spawnPosition = Calculate_WorldSpawnPosition(ownerTransform, _localOffset);
    Vec3 spawnDirection = (_useOwnerForward)
        ? ownerTransform->Get_WorldForward()
        : Vec3::Forward;

    auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
    Shared<GameObject> lockedTarget = nullptr;

    if (myPlayer)
    {
        auto targetCom = myPlayer->Get_Component<TargetComponent>();
        if (targetCom && targetCom->IsLockOn())
            lockedTarget = targetCom->Get_LockedTarget().lock();
    }

    if (_spawnAtLockedTarget && lockedTarget)
    {
        spawnPosition = lockedTarget->Get_Transform()->Get_WorldPosition() + _targetOffset;
    }

    if (_aimAtTarget && lockedTarget)
    {
        Vec3 targetPos = lockedTarget->Get_Transform()->Get_WorldPosition();
        targetPos.y += 0.5f;

        spawnDirection = targetPos - spawnPosition;

        if (spawnDirection.LengthSquared() > 0.0001f)
            spawnDirection.Normalize();
        else
            spawnDirection = ownerTransform->Get_WorldForward();
    }

    SkillObject::FSkillObjectDesc desc{};
    desc.ownerObject = context.owner->GetSharedPtr<GameObject>();
    desc.collisionPreset = _collisionPreset;
    desc.spawnPosition = spawnPosition;
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;
    desc.direction = spawnDirection;

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
        projectileTransform->LookAt(projectileTransform->Get_WorldPosition() + spawnDirection);
    }

    if (_launchIfProjectile)
    {
        projectile->Launch(spawnDirection);
    }
}

Vec3 AN_SpawnSkill::Calculate_WorldSpawnPosition(Shared<Transform> transform, const Vec3& localOffset)
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
