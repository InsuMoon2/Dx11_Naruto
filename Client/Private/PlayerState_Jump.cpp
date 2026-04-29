#include "pch.h"
#include "PlayerState_Jump.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_Jump::PlayerState_Jump()
{
}

PlayerState_Jump::~PlayerState_Jump()
{
}

void PlayerState_Jump::Enter(PlayerStateMachine* state)
{
	if (!state)
		return;

	auto movement = state->Get_Movement();
	if (!movement)
		return;

    movement->Set_OrientRotationToMovement(true);
	movement->Start_Jump();
    GAME->Play_Sound(L"Jump.wav", ESoundChannel::Player, 0.3f);

	state->Play_AnimState(EPlayerState::Jump);
}

void PlayerState_Jump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;   // 재점프 방지

    if (frame.jumpDash)
    {
        state->End_WireLockOn();
        state->Change_State(EPlayerState::JumpDash);
        return;
    }

    if (frame.jumpDown && movement->Can_DoubleJump())
    {
        cmd.doublejump = true;
        state->Change_State(EPlayerState::DoubleJump);
        return;
    }

    if (frame.wireLockOnDown)
    {
        state->Begin_WireLockOn();
    }

    if (state->Is_WireLockOn() && frame.wireDashStart)
    {
        state->End_WireLockOn();
        state->Change_State(EPlayerState::WireDash);
        return;
    }

    if (frame.attackDown && !movement->Is_OnGround())
    {
        // if (!state->Try_MeleeApproach(EPlayerState::JumpAttack))
        //     state->Change_State(EPlayerState::JumpAttack);
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
    const auto* desc = state->Find_AnimStateDesc(EPlayerState::Jump);

    if (!desc)
    {
        // 입력있으면 Run, 아니면 Idle
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

void PlayerState_Jump::Exit(PlayerStateMachine* state)
{
    if (state)
        state->End_WireLockOn();
}

Shared<PlayerState_Jump> PlayerState_Jump::Create()
{
    return make_shared<PlayerState_Jump>();
}
