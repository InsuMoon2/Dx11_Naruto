#include "pch.h"
#include "AN_LaunchSkill.h"
#include "AnimNotify_Factory.h"
#include "SkillObject_Projectile.h"
#include "GameObject.h"
#include "Object_Manager.h"
#include "SkillComponent.h"

REGISTER_ANIM_NOTIFY(AN_LaunchSkill);
IMPLEMENT_REFLECTION(AN_LaunchSkill);

bool AN_LaunchSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_LaunchSkill";
    info.properties.clear();

    PROPERTY_ENUM_JSON("발사 스킬 타입", "launch_type", _launchObjectType, Protocol::OBJECT_TYPE);
    return true;
}

void AN_LaunchSkill::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    CHECK_NULL(skillCom);

    Vec3 lookDir = context.owner->Get_Transform()->Get_WorldForward();

    skillCom->Launch_PendingSkill(_launchObjectType, lookDir);
}
