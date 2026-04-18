#include "pch.h"
#include "PlayerState_Shuriken.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"
#include "SkillComponent.h"

PlayerState_Shuriken::PlayerState_Shuriken()
{
}

PlayerState_Shuriken::~PlayerState_Shuriken()
{
}

void PlayerState_Shuriken::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto owner = state->Get_Owner();
    auto movement = state->Get_Movement();
    auto input = state->Get_Input();
    if (!owner || !movement || !input)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    auto skillCom = owner ? owner->Get_Component<SkillComponent>() : nullptr;
    if (skillCom)
    {
        skillCom->Start_SubSkillCooldown(ESubSkillType::Shuriken);
    }


    state->Play_AnimState(EPlayerState::Shuriken);
}

void PlayerState_Shuriken::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    const auto& frame = input->Get_Frame();

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    if (state->Is_AnimStateFinished())
    {
        if (input->Has_MoveInput())
        {
            state->Change_State(EPlayerState::Run);

            return;
        }

        else
        {
            state->Change_State(EPlayerState::Idle);

            return;
        }

    }

}

void PlayerState_Shuriken::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_GravityEnabled(true);
        movement->Set_OrientRotationToMovement(true);
    }

    auto input = state->Get_Input();
    if (input)
    {
        input->Set_InputMode(EPlayerInputMode::Normal);
    }
}

Shared<PlayerState_Shuriken> PlayerState_Shuriken::Create()
{
    return make_shared<PlayerState_Shuriken>();
}
