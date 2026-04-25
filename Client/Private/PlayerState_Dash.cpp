#include "pch.h"
#include "PlayerState_Dash.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "transform.h"
#include "GameObject.h"

PlayerState_Dash::PlayerState_Dash()
{
}

PlayerState_Dash::~PlayerState_Dash()
{
}

void PlayerState_Dash::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    EMoveInputDirection resolvedDir = EMoveInputDirection::Forward;
    Vec3 resolvedWorldDir = Vec3::Forward;

    const auto& frame = input->Get_Frame();

    state->Set_CameraRelativeMoveDirection(
        Vec2(frame.moveX, frame.moveY),
        resolvedDir,
        resolvedWorldDir);

    _dashDir = resolvedDir;

    auto cmd = state->Init_MoveCommand();

    if (resolvedDir == EMoveInputDirection::Forward)
    {
        Vec3 faceDir = resolvedWorldDir;
        faceDir.y = 0.f;

        if (faceDir.LengthSquared() > FLT_EPSILON)
        {
            faceDir.Normalize();

            Quat faceRot = Quat::FromToRotation(Vec3::Backward, faceDir);
            owner->Get_Transform()->Set_WorldRotation(faceRot);
        }   
    }
    else
    {
        Vec3 faceDir = cmd.moveBasisForward;
        faceDir.y = 0.f;

        if (faceDir.LengthSquared() > FLT_EPSILON)
        {
            faceDir.Normalize();

            Quat faceRot = Quat::FromToRotation(Vec3::Backward, faceDir);
            owner->Get_Transform()->Set_WorldRotation(faceRot);
        }
    }


    state->Set_PendingMoveInputDirection(_dashDir);
    state->Set_PendingDashWorldDirection(resolvedWorldDir);

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    // 실제 이동은 ANS_Move로 세팅
    state->Play_DirectionalAnimState(EPlayerState::Dash, _dashDir);
}

void PlayerState_Dash::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    if (!input || !movement)
        return;

    const auto& frame = input->Get_Frame();
    auto cmd = state->Init_MoveCommand();
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    const float currentFrame = state->Get_AnimTrackPositionTicks();
    const float endFrame = state->Get_AnimDurationTicks();

    if (currentFrame >= 25.f)
    {
        if (input->Has_MoveInput())
        {
            state->Change_State(EPlayerState::Run);
        }
        else
        {
            state->Change_State(EPlayerState::Idle);
        }

        return;
    }
}

void PlayerState_Dash::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Stop_Dash();
        movement->Set_OrientRotationToMovement(true);
    }

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);
    
}

EMoveInputDirection PlayerState_Dash::Classify_InputDirection(const Vec2& moveAxis)
{
    // 입력이 없다면 전방처리
    if (moveAxis.LengthSquared() <= FLT_EPSILON)
        return EMoveInputDirection::Forward;

    // 캐릭터 기준 로컬 입력 절대값이 더 큰 축으로 세팅
    if (fabsf(moveAxis.y) >= fabsf(moveAxis.x))
    {
        return (moveAxis.y >= 0.f)
            ? EMoveInputDirection::Forward
            : EMoveInputDirection::Backward;
    }

    return (moveAxis.x >= 0.f)
        ? EMoveInputDirection::Right
        : EMoveInputDirection::Left;
}

Shared<PlayerState_Dash> PlayerState_Dash::Create()
{
    return make_shared<PlayerState_Dash>();
}
