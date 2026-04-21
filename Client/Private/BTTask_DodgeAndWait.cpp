#include "pch.h"
#include "BTTask_DodgeAndWait.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_DodgeAndWait)

bool BTTask_DodgeAndWait::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_DodgeAndWait";

    PROPERTY_STRING_JSON("Dodge Left State", "dodge_left_state", _dodgeLeftState);
    PROPERTY_STRING_JSON("Dodge Right State", "dodge_right_state", _dodgeRightState);
    PROPERTY_BOOL_JSON("Stop Movement", "stop_movement", _stopMovement);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    PROPERTY_BOOL_JSON("Avoid Same Side Twice", "avoid_same_side_twice", _avoidSameSideTwice);
    PROPERTY_STRING_JSON("Last Dodge Side Key", "last_dodge_side_key", _lastDodgeSideKey);

    return true;
}

BTTask_DodgeAndWait::BTTask_DodgeAndWait()
{
}

BTTask_DodgeAndWait::BTTask_DodgeAndWait(const BTTask_DodgeAndWait& rhs)
    : BTTask(rhs)
    , _dodgeLeftState(rhs._dodgeLeftState)
    , _dodgeRightState(rhs._dodgeRightState)
    , _stopMovement(rhs._stopMovement)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _avoidSameSideTwice(rhs._avoidSameSideTwice)
    , _lastDodgeSideKey(rhs._lastDodgeSideKey)
    , _startedDodge(false)
    , _selectedDodgeSide(0)
{
}

void BTTask_DodgeAndWait::Initialize()
{
    BTTask::Initialize();

    _startedDodge = false;
    _selectedDodgeSide = 0;
    _selectedDodgeState.clear();
}

int32 BTTask_DodgeAndWait::Select_DodgeSide(const Shared<Blackboard>& blackboard)
{
    int32 dodgeSide = (Utils::RandomRange(0.f, 2.f) < 1.f) ? 0 : 1;

    if (_avoidSameSideTwice && blackboard && blackboard->HasKey(_lastDodgeSideKey))
    {
        const int32 lastDodgeSide = blackboard->Get_ValueAsInt(_lastDodgeSideKey);

        if (lastDodgeSide == 0 || lastDodgeSide == 1)
            dodgeSide = 1 - lastDodgeSide;
    }

    if (blackboard)
        blackboard->Set_ValueAsInt(_lastDodgeSideKey, dodgeSide);

    return dodgeSide;
}

string BTTask_DodgeAndWait::Resolve_DodgeStateName(int32 dodgeSide) const
{
    return (dodgeSide == 0) ? _dodgeLeftState : _dodgeRightState;
}

EBTNodeResult BTTask_DodgeAndWait::Update(float timeDelta)
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

    if (!_startedDodge)
    {
        _selectedDodgeSide = Select_DodgeSide(blackboard);
        _selectedDodgeState = Resolve_DodgeStateName(_selectedDodgeSide);

        if (_selectedDodgeState.empty())
        {
            _lastResult = EBTNodeResult::Failed;
            return _lastResult;
        }

        if (_stopMovement)
        {
            blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
            blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
            blackboard->Set_ValueAsBool("Sprint", false);
        }

        blackboard->Set_ValueAsString("AnimState", _selectedDodgeState);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);

        _startedDodge = true;
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    if (!animState->Is_CurrentStateFinished())
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    _startedDodge = false;
    _selectedDodgeSide = 0;
    _selectedDodgeState.clear();

    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

Shared<BTTask_DodgeAndWait> BTTask_DodgeAndWait::Create()
{
    return make_shared<BTTask_DodgeAndWait>();
}

Shared<BTNode> BTTask_DodgeAndWait::Clone()
{
    auto clone = make_shared<BTTask_DodgeAndWait>(*this);
    clone->Initialize();

    return clone;
}
