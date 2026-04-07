#include "pch.h"
#include "AN_Gravity.h"
#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "MovementComponent.h"
#include "GameObject.h"

REGISTER_ANIM_NOTIFY(AN_Gravity);
IMPLEMENT_REFLECTION(AN_Gravity);

bool AN_Gravity::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_Gravity";
    info.properties.clear();

    PROPERTY_BOOL_JSON("중력 체크", "check_gravity", _checkGravity);

    return true;
}

void AN_Gravity::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto movement = context.owner->Get_Component<MovementComponent>();
    if (!movement)
        return;

    movement->Set_GravityEnabled(_checkGravity);
}

