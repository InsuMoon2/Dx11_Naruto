#include "pch.h"
#include "PlayerState_DoubleJump.h"

#include "AnimationStateComponent.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_DoubleJump::PlayerState_DoubleJump()
{
}

PlayerState_DoubleJump::~PlayerState_DoubleJump()
{
}

void PlayerState_DoubleJump::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (!movement)
        return;

    movement->Set_OrientRotationToMovement(true);

    movement->Start_DoubleJump();
    state->Play_AnimState(EPlayerState::DoubleJump);
}

void PlayerState_DoubleJump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto anim = state->Get_AnimationState();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    if (frame.jumpDash)
    {
        state->Change_State(EPlayerState::JumpDash);
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

	auto cmd = state->Init_MoveCommand();
	movement->Apply_Command(cmd);
	movement->Update(timeDelta);

    if (movement->Is_WallRunning())
    {
        state->Change_State(EPlayerState::Wall_Run);
        return;
    }

	if (!movement->Is_OnGround())
		return;

	const bool hasInput = input->Has_MoveInput();
	const auto* desc = anim->Find_State("DoubleJump");

	if (!desc)
	{
		state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
		return;
	}

    if (desc->mode == EStateAnimationMode::Sequence)
    {
        if (hasInput)
        {
            state->Change_State(EPlayerState::Run);
            return;
        }

        state->Request_AnimStateEnd();

        // 입력이 없을 때만 End가 끝난 뒤 Idle로 전환
        if (state->Is_AnimSequenceFinished())
        {
            state->Change_State(EPlayerState::Idle);
        }

        return;
    }

	state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
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
