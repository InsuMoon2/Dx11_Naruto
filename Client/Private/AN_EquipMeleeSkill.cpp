#include "pch.h"
#include "AN_EquipMeleeSkill.h"
#include "GameObject_Factory.h"
#include "AnimNotify_Factory.h"
#include "Transform.h"
#include "GameObject.h"
#include "Utils.h"
#include "SkillObject_Projectile.h"
#include "Client_Defines.h"
#include "SkillComponent.h"

REGISTER_ANIM_NOTIFY(AN_EquipMeleeSkill)

IMPLEMENT_REFLECTION(AN_EquipMeleeSkill)

bool AN_EquipMeleeSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_EquipMeleeSkill";
    info.properties.clear();

    PROPERTY_STRING_JSON("장착 뼈", "bone_name", _boneName);
    PROPERTY_ENUM_JSON("스킬 타입", "spawn_type", _spawnObjectType, Protocol::OBJECT_TYPE);
    PROPERTY_ENUM_JSON("충돌 프리셋", "collision_preset", _collisionPreset, Collision_Preset);

    return true;
}

void AN_EquipMeleeSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto ownerTransform = context.owner->Get_Component<Transform>();
    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (!ownerTransform || !skillCom)
        return;

    SkillObject::FSkillObjectDesc desc{};
    desc.collisionPreset = _collisionPreset;
    desc.spawnPosition = ownerTransform->Get_WorldPosition();

    auto spawned = GAME->Clone_And_Add_GameObject(
        ETOI(ELevelType::Static),
        _spawnObjectType,
        GAME->Current_Level(),
        TEXT("Layer_Skill"), &desc);

    auto skill = dynamic_pointer_cast<SkillObject>(spawned);
    CHECK_NULL(skill);
    skill->Set_Owner(context.owner->GetSharedPtr<GameObject>());

    skillCom->Equip_MeleeSkill(_spawnObjectType, skill, _boneName);
}
