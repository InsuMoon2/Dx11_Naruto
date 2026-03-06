#include "pch.h"
#include "PlayerState_Jump.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_Jump::PlayerState_Jump()
{
}

PlayerState_Jump::~PlayerState_Jump()
{
}

void PlayerState_Jump::Enter(PlayerStateMachine* state)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = true;

    movement->Apply_Command(cmd);
    movement->Update(0.f);  // Jump Velocity 설정

    // TODO : PlayerAnimation : Jump

}

void PlayerState_Jump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;   // 재점프 방지

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    // 착지 판정
    if (movement->Is_OnGround())
    {
        bool hasInput = Vec2(frame.moveX, frame.moveY).LengthSquared() > FLT_EPSILON;

        // 입력값이 있으면 Run, 없으면 Idle
        state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
    }
}

void PlayerState_Jump::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_Jump> PlayerState_Jump::Create()
{
    return make_shared<PlayerState_Jump>();
}
