#include "pch.h"
#include "PlayerState_SuperJump.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_SuperJump::PlayerState_SuperJump()
{
}

PlayerState_SuperJump::~PlayerState_SuperJump()
{
}

void PlayerState_SuperJump::Enter(PlayerStateMachine* state)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto desc = movement->Get_MoveDesc();

    float ratio = ::clamp(frame.superJumpCharge / 3.f, 0.f, 1.f);
    float velocity = ::lerp(desc.superJumpMinVelocity, desc.superJumpMaxVelocity, ratio);

    auto cmd = state->Init_MoveCommand();
    cmd.superJumpVelocity = velocity;
    movement->Apply_Command(cmd);
    movement->Update(0.f);
    
    state->Apply_StateAnimation(EPlayerState::SuperJump);
}

void PlayerState_SuperJump::Update(PlayerStateMachine* state, float timeDelta)
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

void PlayerState_SuperJump::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_SuperJump> PlayerState_SuperJump::Create()
{
    return make_shared<PlayerState_SuperJump>();
}
