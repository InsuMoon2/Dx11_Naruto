#include "pch.h"
#include "AN_ChangePlayerState.h"
#include "AnimNotify_Factory.h"
#include "GameObject.h"
#include "PlayerStateMachine.h"

NS_BEGIN(Client)

REGISTER_ANIM_NOTIFY(AN_ChangePlayerState)
IMPLEMENT_REFLECTION(AN_ChangePlayerState)

bool AN_ChangePlayerState::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "AN_ChangePlayerState";
    info.properties.clear();

    PROPERTY_ENUM("전환 상태", _targetState, EPlayerState);

    return true;
}

void AN_ChangePlayerState::Execute(const FAnimNotifyContext& context)
{
    if (context.isPreview || !context.owner)
        return;

    auto stateMachine = context.owner->Get_Component<PlayerStateMachine>();
    if (!stateMachine)
        return;

    const EPlayerState currentState = stateMachine->Get_CurrentStateID();

    if (currentState != EPlayerState::Attack &&
        currentState != EPlayerState::JumpAttack)
    {
        return;
    }

    stateMachine->Change_State(_targetState);
}

NS_END
