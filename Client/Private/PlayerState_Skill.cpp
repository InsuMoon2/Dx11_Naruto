#include "pch.h"
#include "PlayerState_Skill.h"
#include "PlayerStateMachine.h"
#include "InputComponent.h"
#include "MovementComponent.h"
#include "Model.h"
#include "transform.h"
#include "GameObject.h"
#include "AnimationStateComponent.h"
#include "SkillDataManager.h"
#include "TargetComponent.h"
#include "Character.h"
#include "SkillComponent.h"

PlayerState_Skill::PlayerState_Skill()
{
}

void PlayerState_Skill::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    auto transform = owner->Get_Component<Transform>();
    if (!transform)
        return;

    if (skillData && movement)
    {
        bool isAir = !movement->Is_OnGround();
        if (isAir && skillData->airGravityOff)
        {
            movement->Set_GravityEnabled(false);
            Vec3 velocity = movement->Get_Velocity();
            velocity.y = 0.f; 
            movement->Set_Velocity(velocity);
        }
    }

    auto enumName = magic_enum::enum_name(_myStateId);
    string stateName = enumName.empty() ? "" : string(enumName);

    if (!stateName.empty())
        state->Get_AnimationState()->Play_State(stateName);

    _channelingTimer = 0.f;
    _holdReleaseElapsed = 0.f;
    _isEnding        = false;
    _subPhase        = ESkillSubPhase::Charging;

    _startedOnGround = movement->Is_OnGround();
    _hasLanded = false;
    _chargeReady = false;

    _dashStartPos = transform->Get_WorldPosition();
}

void PlayerState_Skill::Update(PlayerStateMachine* state, float timeDelta)
{
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    bool hasDash = skillData ? skillData->hasDashPhase : false;

    auto movement = state->Get_Movement();
    CHECK_NULL(movement);

    if (hasDash)
    {
        switch (_subPhase)
        {
        case ESkillSubPhase::Charging:
            Update_Charging(state, timeDelta);  break;
        case ESkillSubPhase::Dashing:
            Update_Dashing(state, timeDelta);   break;
        case ESkillSubPhase::Attacking:
            Update_Attacking(state, timeDelta); break;
        }
    }
    else
    {
        EAnimPhase phase = state->Get_AnimPhase();

        //// 루프 진입 시
        if (phase == EAnimPhase::Loop && !_isEnding)
        {
            _channelingTimer += timeDelta;

            float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
            bool isHold = skillData ? skillData->isHoldSkill : false;

            bool shouldEnd = false; // 끝내야 하는가?

            if (isHold)
            {
                auto input = state->Get_Input();
                int32 currentSlot = state->Get_ActiveSkillSlot();

                if (!input->Get_Frame().useSkillPress[currentSlot] || _channelingTimer >= maxDuration)
                {
                    shouldEnd = true;
                }

            }
            else
            {
                if (_channelingTimer >= maxDuration)
                {
                    shouldEnd = true;
                }
            }

            if (shouldEnd)
            {
                state->Request_AnimStateEnd();
                _isEnding = true;
            }

        }
    }

    if (!hasDash && state->Is_AnimStateFinished())
    {
        if (movement->Is_OnGround())
        {
            state->Change_State(EPlayerState::Idle);
        }
        else
        {
            state->Change_State(EPlayerState::JumpFall);
        }
    }
        
}

void PlayerState_Skill::Exit(PlayerStateMachine* state)
{
    if (!state) return;

    auto movement = state->Get_Movement();
    if (movement)
    {
        movement->Stop_Dash();
        movement->Set_OrientRotationToMovement(true);

        movement->Set_GravityEnabled(true);
    }

    auto input = state->Get_Input();
    if (input)
        input->Set_InputMode(EPlayerInputMode::Normal);

    _subPhase = ESkillSubPhase::Charging;
    _dashTarget.reset();

    _startedOnGround = false;
    _hasLanded = false;
    _chargeReady = false;
    _holdReleaseElapsed = 0.f;
}

void PlayerState_Skill::Update_Charging(PlayerStateMachine* state, float timeDelta)
{
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    auto movement  = state->Get_Movement();

    if (!movement || !skillData)
        return;

    EAnimPhase phase = state->Get_AnimPhase();

    if (_startedOnGround)
    {
        if (!_chargeReady)
        {
              if (phase != EAnimPhase::Loop)
                return;

            _chargeReady = true;
        }
    }
    else
    {
        if (_hasLanded)
        {
            if (!_chargeReady)
            {
                if (!state->Is_AnimStateFinished())
                    return;

                state->Get_AnimationState()->Play_StateLoopOnly(skillData->animStateName);
                _chargeReady = true;
            }
        }
        else
        {
            if (phase != EAnimPhase::Loop)
                return;

            if (movement->Is_OnGround())
            {
                _hasLanded = true;
                _chargeReady = false;

                if (!skillData->airLandedAnimStateName.empty())
                {
                    state->Get_AnimationState()->Play_State(skillData->airLandedAnimStateName);
                    return;
                }

                state->Get_AnimationState()->Play_StateLoopOnly(skillData->animStateName);
                _chargeReady = true;
            }
            else
            {
                _chargeReady = true;
            }
        }
    }

    if (!_chargeReady)
        return;

    if (movement->Is_GravityEnabled())
    {
        auto cmd = state->Init_MoveCommand();
        cmd.moveAxis = Vec2::Zero;       
        movement->Apply_Command(cmd);
        movement->Update(timeDelta);     // 중력 적용되게
    }

    _channelingTimer += timeDelta;

    float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
    bool  isHold      = skillData ? skillData->isHoldSkill     : false;
    bool  shouldEnd   = false;

    if (isHold)
    {
        auto input = state->Get_Input();
        int32 currentSlot = state->Get_ActiveSkillSlot();

        const bool isHoldingSkillKey =
            input &&
            currentSlot >= 0 &&
            currentSlot < SkillComponent::Get_SlotCount() &&
            input->Get_Frame().useSkillPress[currentSlot];

        if (isHoldingSkillKey)
        {
            _holdReleaseElapsed = 0.f;
        }
        else
        {
            _holdReleaseElapsed += timeDelta;
        }

        if (_holdReleaseElapsed >= 0.12f || _channelingTimer >= maxDuration)
            shouldEnd = true;
    }
    else
    {
        if (_channelingTimer >= maxDuration)
            shouldEnd = true;
    }

    if (shouldEnd)
    {
        state->Request_AnimStateEnd();
        Begin_DashPhase(state);
    }
}

void PlayerState_Skill::Update_Dashing(PlayerStateMachine* state, float timeDelta)
{
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    if (!skillData) return;

    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    auto transform = owner ? owner->Get_Component<Transform>() : nullptr;

    if (!movement || !transform)
        return; 

    auto cmd = state->Init_MoveCommand();
    cmd.moveAxis = Vec2::Zero;
    movement->Apply_Command(cmd);
    movement->Update(timeDelta);

    const bool useGroundFlow = _startedOnGround || _hasLanded;

    Vec3 currentPos = transform->Get_WorldPosition();
    bool shouldAttack = false;

    // 대 대쉬 거리 도달 체크
    {
        Vec3 travelVec = currentPos - _dashStartPos;
        float travelDistance = travelVec.Length();

        if (travelDistance >= skillData->maxDashDistance)
            shouldAttack = true;
    }   

    if (!shouldAttack)
    {
        auto target = _dashTarget.lock();
        if (target)
        {
            Vec3 toTargetVec = target->Get_Transform()->Get_WorldPosition() - currentPos;

            Vec3 distCheckVec = toTargetVec;
            if (useGroundFlow)
                distCheckVec.y = 0.f;

            float finalStopDist = skillData->targetStopDistance;
            if (!useGroundFlow)
                finalStopDist *= 0.9f;

            if (!skillData->ignoreTargetStop)
            {
                float distToTarget = distCheckVec.Length();
                if (distToTarget <= finalStopDist)
                    shouldAttack = true;
            }

            Vec3 toTargetDir = toTargetVec;
            if (useGroundFlow)
                toTargetDir.y = 0.f;

            toTargetDir = Utils::Safe_Normalize(toTargetDir, _dashDirection);

            bool shouldUpdateDashDirection = true;

            if (skillData->ignoreTargetStop)
            {
                // 내적값이 0보다 작으면 뒤로 판정, 넘어가면 회전 안하게
                const float forwardDot = _dashDirection.Dot(toTargetDir);
                if (forwardDot < 0.f)
                    shouldUpdateDashDirection = false;
            }

            if (shouldUpdateDashDirection)
            {
                _dashDirection = toTargetDir;
            }

            Vec3 lookDir = _dashDirection;
            lookDir.y = 0.f;  
            lookDir.Normalize();
            if (lookDir.LengthSquared() > FLT_EPSILON)
            {
                transform->LookAt(transform->Get_WorldPosition() + lookDir);
            }
        }
    }

    if (shouldAttack)
        Begin_AttackPhase(state);
}


void PlayerState_Skill::Update_Attacking(PlayerStateMachine* state, float timeDelta)
{
    if (!state)
      return;

    auto movement = state->Get_Movement();
    if (!movement)
        return;

    if (!state->Is_AnimStateFinished())
        return;

    if (movement->Is_OnGround())
    {
        state->Change_State(EPlayerState::Idle);
    }
    else
    {
        state->Change_State(EPlayerState::JumpFall);
    }
}

void PlayerState_Skill::Begin_DashPhase(PlayerStateMachine* state)
{
    _subPhase = ESkillSubPhase::Dashing;

    auto owner = state->Get_Owner();
    auto transform = owner ? owner->Get_Component<Transform>() : nullptr;
    auto movement = state->Get_Movement();
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);

    if (!transform || !movement || !skillData)
        return;

    _dashStartPos = transform->Get_WorldPosition();

    const bool useGroundFlow = _startedOnGround || _hasLanded;

    Find_DashTarget(state);

    auto target = _dashTarget.lock();
    if (target)
    {
        auto targetTransform = target->Get_Component<Transform>();
        if (targetTransform)
        {
            _dashDirection = targetTransform->Get_WorldPosition() - _dashStartPos;
        }
    }
    else
    {
        _dashDirection = transform->Get_WorldForward();
        if (_dashDirection.LengthSquared() <= FLT_EPSILON)
            _dashDirection = Vec3::Forward;
    }

    if (useGroundFlow || !target)
    {
        _dashDirection.y = 0.f;
    }


    _dashDirection.Normalize();

    Vec3 lookDir = _dashDirection;
    lookDir.y = 0.f; // 몸통 회전 X
    lookDir.Normalize();

    if (lookDir.LengthSquared() > FLT_EPSILON)
    {
        transform->LookAt(_dashStartPos + lookDir);
    }

    float duration = skillData->maxDashDistance / skillData->dashSpeed;
    movement->Start_Dash(_dashDirection, skillData->maxDashDistance, duration);
}

void PlayerState_Skill::Begin_AttackPhase(PlayerStateMachine* state)
{
    _subPhase = ESkillSubPhase::Attacking;

    // 대쉬 멈추고,
    auto movement = state->Get_Movement();
    if (movement)
        movement->Stop_Dash();

    // Attack End 애니메이션 재생
    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    if (!skillData) return;

     const bool useGroundFlow = _startedOnGround || _hasLanded;

  const string& endStateName = useGroundFlow
        ? skillData->attackEndAnimStateName
        : skillData->airAttackEndAnimStateName;

    if (!endStateName.empty())
    {
        state->Get_AnimationState()->Play_State(endStateName);
    }

}

void PlayerState_Skill::Find_DashTarget(PlayerStateMachine* state)
{
    _dashTarget.reset();

    auto owner = state->Get_Owner();

    auto targetCom = owner->Get_Component<TargetComponent>();
    CHECK_NULL(targetCom);

    if (targetCom->IsLockOn())
    {
        _dashTarget = targetCom->Get_LockedTarget();
    }
}

Shared<PlayerState_Skill> PlayerState_Skill::Create(int32 skill_Id, EPlayerState stateId)
{
    auto state = make_shared<PlayerState_Skill>();

    state->_mySkill_Id = skill_Id;
    state->_myStateId = stateId;

    return state;
}
