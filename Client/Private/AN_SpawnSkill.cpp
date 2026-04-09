#include "pch.h"
#include "AN_SpawnSkill.h"
#include "GameObject_Factory.h"
#include "AnimNotify_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject_Projectile.h"
#include "Client_Defines.h"
#include "MyPlayer.h"
#include "SkillComponent.h"
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

    SkillObject_Projectile::FProjectileSkillDesc desc;
    desc.collisionPreset = _collisionPreset; 
    desc.startAttached = true;
    desc.spawnPosition = Calculate_WorldSpawnPosition(ownerTransform, _localOffset);
    desc.spawnRotation = Vec3::Zero;
    desc.scale = Vec3::One;

    Vec3 launchDir = (_useOwnerForward) ? ownerTransform->Get_WorldForward() : Vec3::Forward;

    if (_aimAtTarget)
    {
        auto myPlayer = dynamic_cast<MyPlayer*>(context.owner);
        if (myPlayer)
        {
            auto targetCom = myPlayer->Get_Component<TargetComponent>();
            if (targetCom && targetCom->IsLockOn())
            {
                auto lockedTarget = targetCom->Get_LockedTarget().lock();
                if (lockedTarget)
                {
                    Vec3 targetPos = lockedTarget->Get_Transform()->Get_WorldPosition();
                    targetPos.y += 0.5f; //
                    launchDir = targetPos - desc.spawnPosition;
                    launchDir.Normalize();
                }
            }
        }
    }

    desc.direction = launchDir;

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        TEXT("Layer_Skill"),
        &desc);

    auto skill = dynamic_pointer_cast<SkillObject_Projectile>(spawned);
    if (skill)
    {
        skill->Set_Owner(context.owner->GetSharedPtr<GameObject>());

        auto projTransform = skill->Get_Transform();
        projTransform->LookAt(projTransform->Get_WorldPosition() + launchDir);

        skill->Launch(launchDir);
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
