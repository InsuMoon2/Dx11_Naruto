#include "pch.h"
#include "PlayerState_Run.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_Run::PlayerState_Run()
{
}

PlayerState_Run::~PlayerState_Run()
{
}

void PlayerState_Run::Enter(PlayerStateMachine* state)
{
    // TODO : PlayerAnimation : Run
}

void PlayerState_Run::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    if (frame.jumpDown)
    {
        state->Change_State(EPlayerState::Jump);
        return;
    }

    if (Vec2(frame.moveX, frame.moveY).LengthSquared() < FLT_EPSILON)
    {
        state->Change_State(EPlayerState::Idle);
        return;
    }

    auto cmd = state->Init_MoveCommand();

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);
}

void PlayerState_Run::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_Run> PlayerState_Run::Create()
{
    return make_shared<PlayerState_Run>();
}
