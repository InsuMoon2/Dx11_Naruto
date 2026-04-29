#include "pch.h"
#include "PlayerState_AirApproach.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"
#include "WireMeshEffect.h"

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

    // AirApproach 진입 시에도 지상 Dash와 같은 진입 사운드를 한 번 재생해 이동 피드백을 맞춘다.
    GAME->Play_Sound(L"Dash.wav", ESoundChannel::Player, 0.3f);

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

        Destroy_WireMesh();

        switch (_approachDesc.arriveAction)
        {
        case EArriveAction::WallAttach:
            {
            Vec3 wallNormal = Utils::Safe_Normalize(_approachDesc.wallNormal, Vec3::Backward);

            MovementComponent::FSurfaceHit wallHit{};
            wallHit.hitNormal = wallNormal;
            wallHit.hitPoint = _approachDesc.targetPosition - wallNormal * movement->Get_MoveDesc().wallAttachOffset;
            wallHit.isValid = true;

            transform->Set_WorldPosition(_approachDesc.targetPosition);
            movement->Enter_WallRun(wallHit);

            state->Change_State(EPlayerState::Wall_Idle);
            return;
            }

        case EArriveAction::GroundLand:
            transform->Set_WorldPosition(_approachDesc.targetPosition);
            movement->Set_Velocity(Vec3::Zero);
            state->Change_State(EPlayerState::JumpFall);
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

    // 있어야한다.
    if (_elapsedTime >= _approachDesc.maxApproachTime)
    {
        Destroy_WireMesh();

        state->Change_State(_approachDesc.nextStateOnFail);
        return;
    }

    Vec3 moveDir = Utils::Safe_Normalize(toTarget, Vec3::Forward);

    Vec3 lookDir = moveDir;
    lookDir.y = 0.f;

    if (lookDir.LengthSquared() > 0.0001f)
    {
        lookDir.Normalize();
        transform->LookAt(currentPos + lookDir);
    }

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
    Destroy_WireMesh();

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

void PlayerState_AirApproach::Destroy_WireMesh()
{
    auto wireEffect = _approachDesc.wireMeshEffect.lock();
    if (wireEffect)
        wireEffect->Set_Destroy(true);

    _approachDesc.wireMeshEffect.reset();
}

Shared<PlayerState_AirApproach> PlayerState_AirApproach::Create()
{
    return make_shared<PlayerState_AirApproach>();
}
