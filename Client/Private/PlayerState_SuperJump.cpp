#include "pch.h"
#include "PlayerState_SuperJump.h"

#include "GameObject.h"
#include "GameInstance.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"

PlayerState_SuperJump::PlayerState_SuperJump()
{
}

PlayerState_SuperJump::~PlayerState_SuperJump()
{
}

void PlayerState_SuperJump::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!movement || !input)
        return;

    // 점프 Velocity값 가져오기
    const float velocity = state->Get_PendingSuperJumpVeloicty();
    movement->Start_SuperJump(velocity);
    movement->Set_OrientRotationToMovement(true);

    GAME->Set_BlurStrength(0.5f);
    GAME->Set_BlurDirection(Vec2(0.f, 1.f));

    state->Play_AnimState(EPlayerState::SuperJump);
}

void PlayerState_SuperJump::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    const auto& frame = input->Get_Frame();

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;

    auto owner = state->Get_Owner();
    auto transform = owner->Get_Transform();
    CHECK_NULL(transform);

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

    // 착지 후 HeightLand에서 사용할 방향 미리 세팅
    Vec3 launchDir = transform->Get_WorldForward();
    launchDir.y = 0.f;

    if (launchDir.LengthSquared() > FLT_EPSILON)
        launchDir.Normalize();
    else
        launchDir = Vec3::Forward;

    state->Set_PendingLandingDir(launchDir);

    if (!movement->Is_OnGround())
        return;

    state->Change_State(EPlayerState::HeightLand);
}

void PlayerState_SuperJump::Exit(PlayerStateMachine* state)
{
    GAME->Set_BlurStrength(0.f);
}

Shared<PlayerState_SuperJump> PlayerState_SuperJump::Create()
{
    return make_shared<PlayerState_SuperJump>();
}
