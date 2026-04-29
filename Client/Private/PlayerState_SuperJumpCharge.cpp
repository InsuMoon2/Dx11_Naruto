#include "pch.h"
#include "PlayerState_SuperJumpCharge.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_SuperJumpCharge::PlayerState_SuperJumpCharge()
{
}

PlayerState_SuperJumpCharge::~PlayerState_SuperJumpCharge()
{
}

void PlayerState_SuperJumpCharge::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (!movement)
        return;

    _shouldPlayChargeEndSound = false;
    movement->Set_OrientRotationToMovement(false);
    GAME->Play_LoopSound(_chargingLoopSoundFile, ESoundChannel::Player, 0.3f, false, 0.f);

    state->Play_AnimState(EPlayerState::SuperJumpCharge);
}

void PlayerState_SuperJumpCharge::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();

    cmd.jump = false;
    cmd.doublejump = false;
    cmd.moveAxis = Vec2::Zero;

    if (frame.superJumpUp && frame.superJumpCharge > 0.f)
    {
        const auto& moveDesc = movement->Get_MoveDesc();

        // 최소 0초, 최대 3초
        const float ratio = ::clamp(frame.superJumpCharge / 3.f, 0.f, 1.f);

        const float velocity = ::lerp(
            moveDesc.superJumpMinVelocity,
            moveDesc.superJumpMaxVelocity, ratio);

        _shouldPlayChargeEndSound = true;
        state->Set_PendingSuperJumpVelocity(velocity);
        state->Change_State(EPlayerState::SuperJump);

        return;
    }

    // 차징 예외처리
    if (!frame.superJumpPress && frame.superJumpCharge <= FLT_EPSILON)
    {
        state->Change_State(EPlayerState::Idle);
        return;
    }
}

void PlayerState_SuperJumpCharge::Exit(PlayerStateMachine* state)
{
    GAME->Stop_Sound(_chargingLoopSoundFile);

    if (_shouldPlayChargeEndSound)
    {
        GAME->Play_Sound(_chargingEndSoundFile, ESoundChannel::Player, 0.35f);
        _shouldPlayChargeEndSound = false;
    }
}

Shared<PlayerState_SuperJumpCharge> PlayerState_SuperJumpCharge::Create()
{
    return make_shared<PlayerState_SuperJumpCharge>();
}
