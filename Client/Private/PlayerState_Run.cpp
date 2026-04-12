#include "pch.h"
#include "PlayerState_Run.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_Run::PlayerState_Run()
{
}

PlayerState_Run::~PlayerState_Run()
{
}

void PlayerState_Run::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_OrientRotationToMovement(true);
    }

    state->Play_AnimState(EPlayerState::Run);
}

void PlayerState_Run::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    const auto& frame = input->Get_Frame();
    auto cmd = state->Init_MoveCommand();

    if (frame.superJumpPress && frame.superJumpCharge > 0.f)
    {
        state->Change_State(EPlayerState::SuperJumpCharge);
        return;
    }

    if (frame.jumpDown)
    {
        movement->Apply_Command(cmd);
        movement->Update(timeDelta);
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
        if (!state->Try_MeleeApproach(EPlayerState::Attack))
            state->Change_State(EPlayerState::Attack);

        return;
    }

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (movement->Is_WallRunning())
    {
        state->Change_State(EPlayerState::Wall_Run);
        return;
    }

    const auto* desc = state->Find_AnimStateDesc(EPlayerState::Run);
    if (!desc)
    {
        state->Change_State(EPlayerState::Idle);
        return;
    }

    if (input->Has_MoveInput())
    {
        if (desc->mode == EStateAnimationMode::Sequence &&
                state->Get_AnimPhase() == EAnimPhase::End)
        {
            FAnimationClipSetting startClip = desc->start;
            startClip.animationName.clear();

            // RunEnd까지 갔다가 다시 이동이 들어오면, Start 생략 후, Loop로 복귀
            state->Play_AnimStateLoopOnly(EPlayerState::Run);
        }

        return;
    }

    if (desc->mode == EStateAnimationMode::Sequence)
    {
        // 입력이 끊기면 RunEnd, End가 끝나야 Idle로 넘어가기
        state->Request_AnimStateEnd();

        if (state->Is_AnimSequenceFinished())
        {
            state->Change_State(EPlayerState::Idle);
        }

        return;
    }

    state->Change_State(EPlayerState::Idle);
}

void PlayerState_Run::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_OrientRotationToMovement(false);
    }
}

Shared<PlayerState_Run> PlayerState_Run::Create()
{
    return make_shared<PlayerState_Run>();
}
