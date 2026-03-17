#include "pch.h"
#include "PlayerState_DoubleJump.h"
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
    state->Apply_StateAnimation(EPlayerState::DoubleJump);
}

void PlayerState_DoubleJump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
	auto model = state->Get_Model();
    auto movement = state->Get_Movement();

	auto cmd = state->Init_MoveCommand();
	movement->Apply_Command(cmd);
	movement->Update(timeDelta);

	if (!movement->Is_OnGround())
		return;

	const bool hasInput = input->Has_MoveInput();
	const auto* desc = state->Find_StateAnimation(EPlayerState::DoubleJump);

	if (!desc)
	{
		state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
		return;
	}

	if (desc->mode == EStateAnimationMode::Sequence)
	{
		model->Request_AnimEnd();

		if (model->Is_AnimationSequenceFinished())
		{
			state->Change_State(hasInput ? EPlayerState::Run : EPlayerState::Idle);
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
