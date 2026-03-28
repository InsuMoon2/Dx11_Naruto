#include "pch.h"
#include "PlayerState_HeightLand.h"
#include "GameObject.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"

PlayerState_HeightLand::PlayerState_HeightLand()
{
}

PlayerState_HeightLand::~PlayerState_HeightLand()
{
}

void PlayerState_HeightLand::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!movement || !owner || !input)
        return;

    auto transform = owner->Get_Transform();
    CHECK_NULL(transform);

    // 구르기 방향 가져오기
    _endHoldTime = 0.f;
    _rollDirection = state->Get_PendingLandingDir();
    _rollDirection.Normalize();

    movement->Set_OrientRotationToMovement(false);

    state->Play_AnimState(EPlayerState::HeightLand);
}

void PlayerState_HeightLand::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    auto transform = owner->Get_Component<Transform>();
    if (!transform)
        return;

    const float currentFrame = state->Get_AnimTrackPositionTicks();
    const float endFrame = state->Get_AnimDurationTicks();

    if (currentFrame < endFrame - 7.f)
    {
        input->Set_InputMode(EPlayerInputMode::LookOnly);

        auto cmd = state->Init_MoveCommand();
        movement->Apply_Command(cmd);
        movement->Update(timeDelta);

        transform->Add_WorldOffset(_rollDirection * _moveSpeed * timeDelta);
    }
    else
    {
        input->Set_InputMode(EPlayerInputMode::Normal);

        auto cmd = state->Init_MoveCommand();
        movement->Apply_Command(cmd);
        movement->Update(timeDelta);

        if (input->Has_MoveInput())
        {
            state->Change_State(EPlayerState::Run);
        }
        else
        {
            state->Change_State(EPlayerState::Idle);
        }
    }
}

void PlayerState_HeightLand::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_OrientRotationToMovement(true);
    }

    _rollDirection = Vec3::Zero;
    _endHoldTime = 0.f;
}

Shared<PlayerState_HeightLand> PlayerState_HeightLand::Create()
{
    return make_shared<PlayerState_HeightLand>();
}
