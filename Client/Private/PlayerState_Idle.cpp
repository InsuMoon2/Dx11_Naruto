#include "pch.h"
#include "PlayerState_Idle.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"

PlayerState_Idle::PlayerState_Idle()
{
}

PlayerState_Idle::~PlayerState_Idle()
{
}

void PlayerState_Idle::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    state->Play_AnimState(EPlayerState::Idle);
}

void PlayerState_Idle::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    if (frame.superJumpPress && frame.superJumpCharge > 0.f)
    {
        state->Change_State(EPlayerState::SuperJumpCharge);
        return;
    }

    if (frame.jumpDown)
    {
        state->Change_State(EPlayerState::Jump);
        return;
    }

    if (frame.dashDown)
    {
        state->Change_State(EPlayerState::Dash);
        return;
    }

    if (frame.attackDown)
    {
        // 이미 근접하거나 타겟이 없으면 공격
        if (!state->Try_MeleeApproach(EPlayerState::Attack))
            state->Change_State(EPlayerState::Attack);

        return;
    }

    if (input->Has_MoveInput())
    {
        state->Change_State(EPlayerState::Run);
        return;
    }

    // 감속 처리용
    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (!movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::JumpFall);
        return;
    }

}

void PlayerState_Idle::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_Idle> PlayerState_Idle::Create()
{
    return make_shared<PlayerState_Idle>();
}
