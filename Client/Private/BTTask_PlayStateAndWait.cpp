#include "pch.h"
#include "BTTask_PlayStateAndWait.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "GameObject.h"

IMPLEMENT_REFLECTION(BTTask_PlayStateAndWait)

bool BTTask_PlayStateAndWait::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_PlayStateAndWait";

    PROPERTY_STRING_JSON("State Name", "state_name", _stateName);
    PROPERTY_BOOL_JSON("Stop Movement", "stop_movement", _stopMovement);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    PROPERTY_STRING_JSON("Set True Flag Key", "set_true_flag_key", _setTrueFlagKey);
    PROPERTY_STRING_JSON("Set False Flag Key", "set_false_flag_key", _setFalseFlagKey);
    PROPERTY_BOOL_JSON("Hold On Finished", "hold_on_finished", _holdOnFinished);

    return true;
}

BTTask_PlayStateAndWait::BTTask_PlayStateAndWait()
{
}

BTTask_PlayStateAndWait::BTTask_PlayStateAndWait(const BTTask_PlayStateAndWait& rhs)
    : BTTask(rhs)
    , _stateName(rhs._stateName)
    , _stopMovement(rhs._stopMovement)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _setTrueFlagKey(rhs._setTrueFlagKey)
    , _setFalseFlagKey(rhs._setFalseFlagKey)
    , _holdOnFinished(rhs._holdOnFinished)
    , _startedPlay(false)
    , _completionApplied(false)
{
}

void BTTask_PlayStateAndWait::Initialize()
{
    BTTask::Initialize();

    // 태스크가 다시 선택될 수 있으므로 매 진입마다 실행 상태를 초기화
    _startedPlay = false;
    _completionApplied = false;
}

EBTNodeResult BTTask_PlayStateAndWait::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto animState = owner->Get_Component<AnimationStateComponent>();
    if (!animState)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    if (!_startedPlay)
    {
        if (_stopMovement)
        {
            blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
            blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
            blackboard->Set_ValueAsBool("Sprint", false);
        }

        blackboard->Set_ValueAsString("AnimState", _stateName);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);

        _startedPlay = true;

        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    if (!animState->Is_CurrentStateFinished())
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    if (!_completionApplied)
    {
        if (!_setTrueFlagKey.empty())
            blackboard->Set_ValueAsBool(_setTrueFlagKey, true);

        if (!_setFalseFlagKey.empty())
            blackboard->Set_ValueAsBool(_setFalseFlagKey, false);

        _completionApplied = true;
    }

    if (_holdOnFinished)
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    _startedPlay = false;
    _completionApplied = false;

    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

Shared<BTTask_PlayStateAndWait> BTTask_PlayStateAndWait::Create()
{
    return make_shared<BTTask_PlayStateAndWait>();
}

Shared<BTNode> BTTask_PlayStateAndWait::Clone()
{
    auto clone = make_shared<BTTask_PlayStateAndWait>(*this);
    clone->Initialize();

    return clone;
}
