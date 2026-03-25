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

PlayerState_Skill::PlayerState_Skill()
{
}

void PlayerState_Skill::Enter(PlayerStateMachine* state)
{
    if (!state)
        return;

    auto input = state->Get_Input();
    auto movement = state->Get_Movement();
    auto owner = state->Get_Owner();
    if (!input || !movement || !owner)
        return;

    input->Set_InputMode(EPlayerInputMode::LookOnly);
    movement->Set_OrientRotationToMovement(false);

    auto transform = owner->Get_Component<Transform>();
    if (!transform)
        return;

    auto enumName = magic_enum::enum_name(_myStateId);
    string stateName = enumName.empty() ? "" : string(enumName);

    if (!stateName.empty())
    {
        state->Get_AnimationState()->Play_State(stateName);
    }

    _channelingTimer = 0.f;
    _isEnding = false;
}

void PlayerState_Skill::Update(PlayerStateMachine* state, float timeDelta)
{
    EAnimPhase phase = state->Get_AnimPhase();

    // 루프 진입 시
    if (phase == EAnimPhase::Loop && !_isEnding)
    {
        _channelingTimer += timeDelta;

        auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(_mySkill_Id);
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

    if (state->Is_AnimSequenceFinished())
    {
        state->Change_State(EPlayerState::Idle);
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
    }

    auto input = state->Get_Input();
    input->Set_InputMode(EPlayerInputMode::Normal);

}

Shared<PlayerState_Skill> PlayerState_Skill::Create(int32 skill_Id)
{
    auto state = make_shared<PlayerState_Skill>();

    state->_mySkill_Id = skill_Id;

    auto skillData = GET_SINGLE(SkillDataManager)->Get_SkillData(skill_Id);
    if (skillData)
    {
        auto enumVal = magic_enum::enum_cast<EPlayerState>(skillData->animStateName);
        if (enumVal.has_value())
        {
            state->_myStateId = enumVal.value();
        }
    }

    return state;
}
