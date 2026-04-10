#include "pch.h"
#include "AN_ClearMeleeSkill.h"
#include "GameObject.h"
#include "SkillComponent.h"
#include "AnimNotify_Factory.h"

REGISTER_ANIM_NOTIFY(AN_ClearMeleeSkill)
IMPLEMENT_REFLECTION(AN_ClearMeleeSkill)

bool AN_ClearMeleeSkill::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_ClearMeleeSkill";
    info.properties.clear();

    return true;
}

void AN_ClearMeleeSkill::Execute(const FAnimNotifyContext& context)
{
      if (context.isPreview || !context.owner)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    
    if (skillCom)
        skillCom->Clear_MeleeSkill();
}
