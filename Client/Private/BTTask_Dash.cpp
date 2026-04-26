#include "pch.h"
#include "BTTask_Dash.h"

#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "AnimationStateComponent.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_Dash)

bool BTTask_Dash::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_Dash";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Left Anim State",   "left_anim_state",   _leftAnimState);
    PROPERTY_STRING_JSON("Right Anim State",  "right_anim_state",  _rightAnimState);
    PROPERTY_BOOL_JSON("Face Target",         "face_target",       _faceTarget);
    PROPERTY_BOOL_JSON("Request Anim End",    "request_anim_end",  _requestAnimEnd);
    PROPERTY_FLOAT_JSON("Dash Duration",      "dash_duration",     _dashDuration, 0.05f, 2.f);
    PROPERTY_FLOAT_JSON("Dash Move Power",    "dash_move_power",   _dashMovePower, 0.f, 3.f);
    PROPERTY_FLOAT_JSON("Backward Blend Power", "backward_blend_power", _backwardBlendPower, 0.f, 3.f);
    PROPERTY_BOOL_JSON("Use Sprint",          "use_sprint",        _useSprint);
    PROPERTY_BOOL_JSON("Stop Movement On Finish", "stop_movement_on_finish", _stopMovementOnFinish);

    return true;
}

BTTask_Dash::BTTask_Dash()
{
}

BTTask_Dash::BTTask_Dash(const BTTask_Dash& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _leftAnimState(rhs._leftAnimState)
    , _rightAnimState(rhs._rightAnimState)
    , _faceTarget(rhs._faceTarget)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _dashDuration(rhs._dashDuration)
    , _dashMovePower(rhs._dashMovePower)
    , _backwardBlendPower(rhs._backwardBlendPower)
    , _useSprint(rhs._useSprint)
    , _stopMovementOnFinish(rhs._stopMovementOnFinish)
    , _startedDash(false)
    , _elapsedDashTime(0.f)
    , _selectedDashSide(rhs._selectedDashSide)
    , _selectedAnimState("")
{
}

void BTTask_Dash::Initialize()
{
    BTTask::Initialize();

    _startedDash = false;
    _elapsedDashTime = 0.f;
    _selectedDashSide = 1;
    _selectedAnimState.clear();
}

EBTNodeResult BTTask_Dash::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner      = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    auto ownerTransform = owner->Get_Component<Transform>();
    auto animState      = owner->Get_Component<AnimationStateComponent>();

    if (!ownerTransform || !animState)
        return _lastResult = EBTNodeResult::Failed;

    if (!_startedDash)
    {
        _selectedDashSide = (Utils::RandomRange(0.f, 1.f) < 0.5f) ? -1 : 1;
        _selectedAnimState = (_selectedDashSide < 0) ? _leftAnimState : _rightAnimState;

        if (_selectedAnimState.empty())
            return _lastResult = EBTNodeResult::Failed;

        if (_faceTarget)
        {
            auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
            if (targetObject && !targetObject->Is_Destroy())
            {
                auto targetTransform = targetObject->Get_Component<Transform>();
                if (targetTransform)
                {
                    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
                    Vec3 toTarget = targetTransform->Get_WorldPosition() - ownerPos;
                    toTarget.y = 0.f;

                    if (toTarget.LengthSquared() > FLT_EPSILON)
                    {
                        toTarget.Normalize();
                        ownerTransform->LookAt(ownerPos + toTarget);
                    }
                }
            }
        }

        blackboard->Set_ValueAsString("AnimState", _selectedAnimState);

        const int32 nextSerial = blackboard->HasKey("AnimReplaySerial")
            ? blackboard->Get_ValueAsInt("AnimReplaySerial") + 1
            : 1;
        blackboard->Set_ValueAsInt("AnimReplaySerial", nextSerial);

        _startedDash = true;
        _elapsedDashTime = 0.f;
    }

    Vec3 sideDirection = ownerTransform->Get_WorldRight();
    sideDirection.y = 0.f;
    sideDirection = Utils::Safe_Normalize(sideDirection, Vec3::Right);
    sideDirection *= static_cast<float>(_selectedDashSide);

    Vec3 backwardDirection = -ownerTransform->Get_WorldForward();
    backwardDirection.y = 0.f;
    backwardDirection = Utils::Safe_Normalize(backwardDirection, Vec3::Backward);

    Vec3 dashDirection = sideDirection * _dashMovePower + backwardDirection * _backwardBlendPower;
    dashDirection.y = 0.f;

    if (dashDirection.LengthSquared() <= FLT_EPSILON)
        dashDirection = sideDirection * _dashMovePower;

    blackboard->Set_ValueAsFloat("MoveAxisX", dashDirection.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", dashDirection.z);
    blackboard->Set_ValueAsBool("Sprint", _useSprint);

    _elapsedDashTime += timeDelta;

    const bool durationFinished = _elapsedDashTime >= _dashDuration;
    const bool animFinished = !_requestAnimEnd || animState->Is_CurrentStateFinished();

    if (!durationFinished || !animFinished)
        return _lastResult = EBTNodeResult::InProgress;

    if (_stopMovementOnFinish)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
    }

    _startedDash = false;
    _elapsedDashTime = 0.f;
    _selectedDashSide = 1;
    _selectedAnimState.clear();
    return _lastResult = EBTNodeResult::Succeeded;
}

Shared<BTTask_Dash> BTTask_Dash::Create()
{
    return make_shared<BTTask_Dash>();
}

Shared<BTNode> BTTask_Dash::Clone()
{
    auto clone = make_shared<BTTask_Dash>(*this);
    clone->Initialize();
    return clone;
}
