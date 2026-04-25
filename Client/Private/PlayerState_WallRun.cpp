#include "pch.h"
#include "PlayerState_WallRun.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_WallRun::PlayerState_WallRun()
{
    
}

PlayerState_WallRun::~PlayerState_WallRun()
{
    
}

void PlayerState_WallRun::Enter(PlayerStateMachine* state)
{
    if (!state) return;
    auto movement = state->Get_Movement();
    if (movement) movement->Set_OrientRotationToMovement(false);

    state->Play_AnimState(EPlayerState::Wall_Run);
}

void PlayerState_WallRun::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement) return;

    const auto& frame = input->Get_Frame();

    if (frame.jumpDown)
    {
        movement->Start_WallJump();
        state->Change_State(EPlayerState::Jump);
        return;
    }

    if (frame.attackDown)
    {
        state->Change_State(EPlayerState::Attack);
        return;
    }

    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (!movement->Is_WallRunning())
    {
        if (movement->Is_OnGround())
        {
            state->Change_State(EPlayerState::Run);
        }
        else if (movement->Try_RecoverWallRunHold())
        {
            state->Change_State(EPlayerState::Wall_Idle);
        }
        else
        {
            state->Change_State(EPlayerState::JumpFall);
        }
        return;
    }

    const auto* desc = state->Find_AnimStateDesc(EPlayerState::Wall_Run);
    if (!desc)
    {
        state->Change_State(EPlayerState::Wall_Idle);
        return;
    }

    if (input->Has_MoveInput())
    {
        if (desc->mode == EStateAnimationMode::Sequence &&
            state->Get_AnimPhase() == EAnimPhase::End)
        {
            state->Play_AnimStateLoopOnly(EPlayerState::Wall_Run);
        }
        return;
    }
    if (desc->mode == EStateAnimationMode::Sequence)
    {
        state->Request_AnimStateEnd();

        if (state->Is_AnimSequenceFinished())
        {
            state->Change_State(EPlayerState::Wall_Idle);
        }
        return;
    }

    state->Change_State(EPlayerState::Wall_Idle);
}

void PlayerState_WallRun::Exit(PlayerStateMachine* state)
{
    
}

Shared<PlayerState_WallRun> PlayerState_WallRun::Create()
{
    return make_shared<PlayerState_WallRun>();
}
