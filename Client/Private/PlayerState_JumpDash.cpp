#include "pch.h"
#include "PlayerState_JumpDash.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "transform.h"
#include "GameObject.h"

PlayerState_JumpDash::PlayerState_JumpDash()
{
}

PlayerState_JumpDash::~PlayerState_JumpDash()
{
}

void PlayerState_JumpDash::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    auto transform = owner->Get_Component<Transform>();
    if (!transform)
        return;

    _dashDir = EMoveInputDirection::Forward;

    _requestedAnimEnd = false;
    _gravityRestored = false;

    state->Set_PendingMoveInputDirection(_dashDir);

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    input->Set_JumpInputEnabled(true);

    movement->Set_OrientRotationToMovement(false);
    movement->Set_GravityEnabled(false); // 공중 대쉬때 중력 끄기

    Vec3 velocity = movement->Get_Velocity();
    velocity.y = 0.f;
    movement->Set_Velocity(velocity);

    Vec3 dashWorldDir = transform->Get_WorldForward();
    dashWorldDir.y = 0.f;

    if (dashWorldDir.LengthSquared() <= FLT_EPSILON)
        dashWorldDir = Vec3::Forward;

    dashWorldDir.Normalize();

    const auto& moveDesc = movement->Get_MoveDesc();
    movement->Start_Dash(dashWorldDir, moveDesc.dashDistance, moveDesc.dashDuration);
    GAME->Play_Sound(L"Dash.wav", ESoundChannel::Player, 0.3f);

    state->Play_AnimState(EPlayerState::JumpDash);
}

void PlayerState_JumpDash::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    const auto& frame = input->Get_Frame();
    auto cmd = state->Init_MoveCommand();

    cmd.jump = false;
    cmd.doublejump = false;
    cmd.superJumpVelocity = 0.f;

    if (frame.jumpDown && movement->Can_DoubleJump())
    {
        cmd.doublejump = true;
        state->Change_State(EPlayerState::DoubleJump);
        return;
    }

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (movement->Is_OnGround())
    {
        bool hasInput = input->Has_MoveInput();
        state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
        return;
    }

    if (state->Is_AnimStateFinished())
    {
        state->Change_State(EPlayerState::JumpFall);
        return;
    }
}

void PlayerState_JumpDash::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Stop_Dash();
        movement->Set_OrientRotationToMovement(true);

        // 상태 끝날 때 무조건 중력 복구
        movement->Set_GravityEnabled(true);
        movement->Reset_DoubleJumpCount();

    }

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

}

Shared<PlayerState_JumpDash> PlayerState_JumpDash::Create()
{
    return make_shared<PlayerState_JumpDash>();
}
