#include "pch.h"
#include "PlayerState_DoubleJump.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_DoubleJump::PlayerState_DoubleJump()
{
}

PlayerState_DoubleJump::~PlayerState_DoubleJump()
{
}

void PlayerState_DoubleJump::Enter(PlayerStateMachine* state)
{
    auto cmd = state->Init_MoveCommand();
    cmd.doublejump = true;

    state->Get_Movement()->Apply_Command(cmd);
    state->Get_Movement()->Update(0.f);  // Jump Velocity 설정

    // TODO : PlayAnimation : Double Jump

}

void PlayerState_DoubleJump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    const auto& frame = input->Get_Frame();

    auto movement = state->Get_Movement();
    auto cmd = state->Init_MoveCommand();

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

void PlayerState_DoubleJump::Exit(PlayerStateMachine* state)
{
    auto cmd = state->Init_MoveCommand();
    cmd.doublejump = false;
}

Shared<PlayerState_DoubleJump> PlayerState_DoubleJump::Create()
{
    return make_shared<PlayerState_DoubleJump>();
}
