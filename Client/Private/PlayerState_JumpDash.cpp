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
    cmd.doublejump = false; // 일단 더블점프도 막기
    cmd.superJumpVelocity = 0.f;

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (!_requestedAnimEnd && movement->Get_DashNormalizedTime() >= 1.f)
    {
        state->Request_AnimStateEnd();
        _requestedAnimEnd = true;
    }

    if (!_gravityRestored && state->Get_AnimPhase() == EAnimPhase::End)
    {
        movement->Set_GravityEnabled(true);

        Vec3 velocity = movement->Get_Velocity();
        velocity.y = 0.f;
        movement->Set_Velocity(velocity);

        _gravityRestored = true;
    }

    // 아직 End Phase 전이면 공중 유지
    if (!_gravityRestored)
        return;

    // 아직 공중상태
    if (!movement->Is_OnGround())
        return;

    // 착지 애니메이션 끝나면 상태 전환
    if (state->Is_AnimSequenceFinished() || state->Is_AnimStateFinished())
    {
        if (input->Has_MoveInput())
        {
            state->Change_State(EPlayerState::Run);
        }
        else
        {
            state->Change_State(EPlayerState::Idle);
        }

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
    }

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);
    
}

Shared<PlayerState_JumpDash> PlayerState_JumpDash::Create()
{
    return make_shared<PlayerState_JumpDash>();
}
