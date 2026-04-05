#include "pch.h"
#include "PlayerState_Wall_Idle.h"

#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_Wall_Idle::PlayerState_Wall_Idle()
{
    
}

PlayerState_Wall_Idle::~PlayerState_Wall_Idle()
{
    
}

void PlayerState_Wall_Idle::Enter(PlayerStateMachine* state)
{
    if (!state) return;
    auto movement = state->Get_Movement();
    if (movement) movement->Set_OrientRotationToMovement(false);

    if (state->Get_PrevStateID() == EPlayerState::Jump ||
        state->Get_PrevStateID() == EPlayerState::DoubleJump ||
        state->Get_PrevStateID() == EPlayerState::JumpFall)
    {
        state->Play_AnimState(EPlayerState::JumpFall);
        state->Request_AnimStateEnd(); 
    }
    else
    {
        state->Play_AnimState(EPlayerState::Wall_Idle);
    }
}

void PlayerState_Wall_Idle::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement) return;

    const auto& frame = input->Get_Frame();

    if (input->Has_MoveInput())
    {
        state->Change_State(EPlayerState::Wall_Run);
        return;
    }

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

    auto idleCmd = state->Init_MoveCommand();
    idleCmd.moveAxis = Vec2::Zero;
    movement->Apply_Command(idleCmd);
    movement->Update(timeDelta);
  
}

void PlayerState_Wall_Idle::Exit(PlayerStateMachine* state) {}

Shared<PlayerState_Wall_Idle> PlayerState_Wall_Idle::Create()
{
    return make_shared<PlayerState_Wall_Idle>();
}
