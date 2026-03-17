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

    state->Apply_StateAnimation(EPlayerState::Run);
}

void PlayerState_Run::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto model = state->Get_Model();
    if (!input || !movement || !model)
        return;

    const auto& frame = input->Get_Frame();

    if (frame.superJumpUp && frame.superJumpCharge > 0.f)
    {
        state->Change_State(EPlayerState::SuperJump);
        return;
    }

    if (frame.jumpDown)
    {
        state->Change_State(EPlayerState::Jump);
        return;
    }

    auto cmd = state->Init_MoveCommand();

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    const auto* desc = state->Find_StateAnimation(EPlayerState::Run);
    if (!desc)
    {
        state->Change_State(EPlayerState::Idle);
        return;
    }

    if (input->Has_MoveInput())
    {
        if (desc->mode == EStateAnimationMode::Sequence &&
                model->Get_AnimPhase() == EAnimPhase::End)
        {
            FAnimationClipSetting startClip = desc->start;
            startClip.animationName.clear();

            model->Set_AnimationSequence(startClip, desc->loop, desc->end);
        }

        return;
    }

    if (desc->mode == EStateAnimationMode::Sequence)
    {
        // 입력이 끊기면 RunEnd, End가 끝나야 Idle로 넘어가기
        model->Request_AnimEnd();

        if (model->Is_AnimationSequenceFinished())
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
