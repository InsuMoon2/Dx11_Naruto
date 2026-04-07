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
    _isEnding = false;
    _subPhase = ESkillSubPhase::Charging;

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

    if (state->Is_AnimStateFinished())
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

    // 장착된 스킬 해제
    auto skillCom = state->Get_Owner()->Get_Component<SkillComponent>();
    if (skillCom)
    {
        skillCom->Clear_MeleeSkill();
    }
}

void PlayerState_Skill::Update_Charging(PlayerStateMachine* state, float timeDelta)
{
    EAnimPhase phase = state->Get_AnimPhase();
    if (phase != EAnimPhase::Loop)
        return; // 아직 Start 재생 중이면 대기

    _channelingTimer += timeDelta;

    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
    float maxDuration = skillData ? skillData->loopDurationSec : 0.f;
    bool isHold = skillData ? skillData->isHoldSkill : false;
    bool shouldEnd = false;

    if (isHold)
    {
        auto input = state->Get_Input();
        int32 currentSlot = state->Get_ActiveSkillSlot();

        if (!input->Get_Frame().useSkillPress[currentSlot] || _channelingTimer >= maxDuration)
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

    if (!skillData || !movement || !transform)
        return;

    auto cmd = state->Init_MoveCommand();
    cmd.moveAxis = Vec2::Zero;
    movement->Apply_Command(cmd);

    movement->Update(timeDelta);

    Vec3 currentPos = transform->Get_WorldPosition();
    bool shouldAttack = false;

    // 최대 거리 도달
    float traveled = Vec3::Distance(currentPos, _dashStartPos);
    if (traveled >= skillData->maxDashDistance)
        shouldAttack = true;

    // 또는 타겟 근처까지 도달
    if (!shouldAttack)
    {
        auto target = _dashTarget.lock();
        if (target)
        {
            float distToTarget = Vec3::Distance(currentPos, target->Get_Transform()->Get_WorldPosition());

            if (distToTarget <= skillData->targetStopDistance)
            {
                shouldAttack = true;
            }

            Vec3 toTarget = target->Get_Transform()->Get_WorldPosition() - transform->Get_WorldPosition();
            toTarget.y = 0.f;
            toTarget = Utils::Safe_Normalize(toTarget, _dashDirection);

            _dashDirection = toTarget;
            transform->LookAt(transform->Get_WorldPosition() + _dashDirection);

        }
    }

    // 이제 여기서 마지막 애니메이션 재생
    if (shouldAttack)
        Begin_AttackPhase(state);
}

void PlayerState_Skill::Update_Attacking(PlayerStateMachine* state, float timeDelta)
{
    // AttackEnd 애니메이션은 Single 일듯 웬만하면
    if (state->Is_AnimStateFinished())
        state->Change_State(EPlayerState::Idle);
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
    _dashDirection.y = 0.f;
    _dashDirection.Normalize();

    transform->LookAt(_dashStartPos + _dashDirection);

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
    if (skillData && !skillData->attackEndAnimStateName.empty())
    {
        // 이름을 데이터랑 Enum값일아 잘 맞춰야함
        state->Get_AnimationState()->Play_State(skillData->attackEndAnimStateName);
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
