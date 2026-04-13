#include "pch.h"
#include "AN_LightningTrail_Stop.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "LightningTrail_Component.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_LightningTrail_Stop)

void AN_LightningTrail_Stop::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto trailCom = context.owner->Get_Component<LightningTrail_Component>();
    if (!trailCom)
        return;

    trailCom->Stop_LightningTrail();
}

NS_END
