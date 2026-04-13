#include "pch.h"
#include "AN_StretchingMesh_End.h"
#include "AnimNotify_Factory.h"
#include "SkillComponent.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_StretchingMesh_End)

void AN_StretchingMesh_End::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto skillCom = context.owner->Get_Component<SkillComponent>();
    if (!skillCom)
        return;

    // 바로 파괴 AttackEnd에서 하면 될듯
    auto activeEffect = skillCom->Get_ActiveStretchingMesh();
    if (activeEffect && !activeEffect->Is_Destroy())
    {
        activeEffect->Set_Destroy(true);
    }
}

NS_END
