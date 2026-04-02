#include "pch.h"
#include "AN_ChangeState.h"
#include "AnimNotify_Factory.h"
#include "GameObject_Factory.h"
#include "PlayerStateMachine.h"
#include "GameObject.h"

REGISTER_ANIM_NOTIFY(AN_ChangeState)
IMPLEMENT_REFLECTION(AN_ChangeState)

bool AN_ChangeState::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_ChangeState";
    info.properties.clear();

    PROPERTY_ENUM("Next State", _nextState, EPlayerState);

    return true;
}

string AN_ChangeState::Get_TypeName() const
{
    return "AN_ChangeState";
}

void AN_ChangeState::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview)
        return;

    if (!context.owner)
        return;

    auto stateMachine = context.owner->Get_Component<PlayerStateMachine>();
    if (stateMachine)
    {
        stateMachine->Force_Enter_State(_nextState);
    }
}
