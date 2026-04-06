#include "pch.h"
#include "PlayerState_AirApproach.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"

PlayerState_AirApproach::PlayerState_AirApproach()
{
}

PlayerState_AirApproach::~PlayerState_AirApproach()
{
}

void PlayerState_AirApproach::Enter(PlayerStateMachine* state)
{
	if (!state)
		return;

    auto owner = state->Get_Owner();
    auto movement = state->Get_Movement();
    auto input = state->Get_Input();
    if (!owner || !movement || !input)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    Vec3 velocity = movement->Get_Velocity();
    velocity.y = 0.f;
    movement->Set_Velocity(velocity);

    if (_approachDesc.gravityOff)
    {
        movement->Set_GravityEnabled(false);
    }

    _elapsedTime = 0.f;
    _arrived = false;

	state->Play_AnimState(_approachDesc.animState);
}

void PlayerState_AirApproach::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    const auto& frame = input->Get_Frame();

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    _elapsedTime += timeDelta;

    Vec3 currentPos = transform->Get_WorldPosition();
    Vec3 toTarget = _approachDesc.targetPosition - currentPos; // 타겟까지 남은 거ㅣㄹ
    const float distance = toTarget.Length();

    if (distance <= _approachDesc.stopDistance)
    {
        _arrived = true;

        switch (_approachDesc.arriveAction)
        {
        case EArriveAction::WallAttach:
            state->Change_State(EPlayerState::Wall_Idle);
            return;

        case EArriveAction::ChangeState:
            if (_approachDesc.nextStateOnArrive != EPlayerState::END)
            {
                state->Change_State(_approachDesc.nextStateOnArrive);
                return;
            }
            break;

        default:
            break;
        }

        state->Change_State(EPlayerState::JumpFall);
        return;
    }

    // 너무 오래 접근하면 탈주처리 -> 일단 기본 JumpFall로 세팅
    if (_elapsedTime >= _approachDesc.maxApproachTime)
    {
        state->Change_State(_approachDesc.nextStateOnFail);
        return;
    }

    // 타겟으로 회전처리
    Vec3 moveDir = Utils::Safe_Normalize(toTarget, Vec3::Forward);
    const Vec3 ownerPos = transform->Get_WorldPosition();
    transform->LookAt(ownerPos + moveDir);

    Vec3 velocity = moveDir * _approachDesc.moveSpeed;
    movement->Set_Velocity(velocity);

    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;
    cmd.doublejump = false;

    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    // 안끝나면 Loop만 재생
    if (state->Get_AnimPhase() == EAnimPhase::End)
    {
        state->Play_AnimStateLoopOnly(_approachDesc.animState);
    }
}

void PlayerState_AirApproach::Exit(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Set_GravityEnabled(true);
        movement->Set_OrientRotationToMovement(true);
    }

    auto input = state->Get_Input();
    if (input)
    {
        input->Set_InputMode(EPlayerInputMode::Normal);
    }
}

Shared<PlayerState_AirApproach> PlayerState_AirApproach::Create()
{
    return make_shared<PlayerState_AirApproach>();
}
