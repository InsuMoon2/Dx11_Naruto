#include "pch.h"
#include "AN_Trail_Stop.h"
#include "AnimNotify_Factory.h"
#include "Trail_Component.h"
#include "GameObject.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_Trail_Stop)

void AN_Trail_Stop::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner) return;

    if (auto trailCom = context.owner->Get_Component<Trail_Component>())
    {
        trailCom->Stop_Trail();
    }
}

NS_END
