#include "pch.h"
#include "PlayerState_JumpFall.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_JumpFall::PlayerState_JumpFall()
{
}

PlayerState_JumpFall::~PlayerState_JumpFall()
{
}

void PlayerState_JumpFall::Enter(PlayerStateMachine* state)
{
	if (!state)
		return;

	auto movement = state->Get_Movement();
	if (!movement)
		return;

    movement->Set_OrientRotationToMovement(true);

    auto previewState = state->Get_PrevStateID();

    state->Play_AnimState(EPlayerState::JumpFall);
    //state->Request_AnimStateEnd();
}

void PlayerState_JumpFall::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;   

    if (frame.jumpDash)
    {
        state->Change_State(EPlayerState::JumpDash);
        return;
    }

    if (frame.jumpDown && movement->Can_DoubleJump())
    {
        cmd.doublejump = true;
        state->Change_State(EPlayerState::DoubleJump);
        return;
    }

    if (frame.wireDash)
    {
        state->Change_State(EPlayerState::WireDash);
        return;
    }

    if (frame.attackDown && !movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::JumpAttack);
        return;
    }

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    if (movement->Is_WallRunning())
    {
        state->Change_State(EPlayerState::Wall_Run);
        return;
    }

    // 아직 공중이면 점프상태 유지
    if (!movement->Is_OnGround())
        return;

    const bool hasInput = input->Has_MoveInput();

    if (hasInput)
    {
        state->Change_State(EPlayerState::Run);
        return;
    }

    state->Request_AnimStateEnd();

    if (state->Is_AnimSequenceFinished())
    {
        state->Change_State(EPlayerState::Idle);
    }
    
}

void PlayerState_JumpFall::Exit(PlayerStateMachine* state)
{

}

Shared<PlayerState_JumpFall> PlayerState_JumpFall::Create()
{
    return make_shared<PlayerState_JumpFall>();
}
