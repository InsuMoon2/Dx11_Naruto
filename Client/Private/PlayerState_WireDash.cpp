#include "pch.h"
#include "PlayerState_WireDash.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "GameObject.h"
#include "Debug_Manager.h"
#include "PlayerState_AirApproach.h"

PlayerState_WireDash::PlayerState_WireDash()
{
}

PlayerState_WireDash::~PlayerState_WireDash()
{
}

void PlayerState_WireDash::Enter(PlayerStateMachine* state)
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

    movement->Set_GravityEnabled(false);
    movement->Set_Velocity(Vec3(movement->Get_Velocity().x, 0.f, movement->Get_Velocity().z));

    input->Set_InputMode(EPlayerInputMode::LookOnly);

    _isWireAttach = false;

	state->Play_AnimState(EPlayerState::WireDash);
}

void PlayerState_WireDash::Update(PlayerStateMachine* state, float timeDelta)
{
    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();

    if (!owner || !movement || !input)
        return;

    auto transform = owner->Get_Transform();
    if (!transform)
        return;

    const auto& frame = input->Get_Frame();
    auto cmd = state->Init_MoveCommand();
    cmd.jump = false;   // 재점프 방지

    // 벽 충돌 판정
    const auto& wireDashDesc = movement->Get_WireDashDesc();

    Vec3 start = transform->Get_WorldPosition();
    start.y += wireDashDesc.traceStartOffsetY;

    Vec3 forward = transform->Get_WorldForward();
    forward = Utils::Safe_Normalize(forward, Vec3::Forward);

    Vec3 end = start + forward * wireDashDesc.maxDistance;

    MovementComponent::FSurfaceHit wallHit{};
    bool isHit = movement->Try_WireDash_WallTrace(start, forward, wallHit);

    // 디버깅 렌더
    {
        FDebugTraceLineDesc traceDesc{};
        traceDesc.start = start;
        traceDesc.end = end;
        traceDesc.isHit = isHit;
        traceDesc.hitPoint = wallHit.hitPoint;
        traceDesc.hitNormal = wallHit.hitNormal;
        traceDesc.duration = 0.f;
        traceDesc.depthEnabled = true;
        traceDesc.drawHitPoint = true;
        traceDesc.drawHitNormal = true;
        traceDesc.drawRemainderOnHit = true;

        GAME->Draw_DebugTraceLine(traceDesc);
    }

    if (!_isWireAttach && isHit)
    {
        // 충돌 판정, 위치 세팅 -> 여기서 이제 AirApproach로 넘기기
        _isWireAttach = true;

        const float attachOffset = movement->Get_MoveDesc().wallAttachOffset;
        _wireAttachPosition = wallHit.hitPoint + wallHit.hitNormal * attachOffset;

        auto airApproach = state->Get_State<PlayerState_AirApproach>(EPlayerState::AirApproach);
        if (airApproach)
        {
            PlayerState_AirApproach::FApproachDesc desc{};
            desc.targetPosition = _wireAttachPosition;
            desc.stopDistance = movement->Get_WireDashDesc().stopDistance;
            desc.moveSpeed = movement->Get_WireDashDesc().approachSpeed;
            desc.maxApproachTime = 0.8f; // 일단 0.8초 -> 이건 WireDash말고 공중에서 접근할 때만 사용할듯
            desc.arriveAction = PlayerState_AirApproach::EArriveAction::WallAttach;
            desc.nextStateOnFail = EPlayerState::JumpFall;
            desc.wallNormal = wallHit.hitNormal;

            airApproach->Set_AirApproachDesc(desc);
        }
    }

    if (state->Is_AnimStateFinished())
    {
        if (_isWireAttach)
        {
            state->Change_State(EPlayerState::AirApproach);
            return;
        }

        state->Change_State(EPlayerState::JumpFall);
        return;
    }
}

void PlayerState_WireDash::Exit(PlayerStateMachine* state)
{
    auto movement = state->Get_Movement();
    auto input = state->Get_Input();
    if (!movement || !input)
        return;

    movement->Set_GravityEnabled(true);
    input->Set_InputMode(EPlayerInputMode::Normal);
}

Shared<PlayerState_WireDash> PlayerState_WireDash::Create()
{
    return make_shared<PlayerState_WireDash>();
}
