#include "pch.h"
#include "BTTask_RetreatAndWait.h"

#include "AnimationStateComponent.h"
#include "Blackboard.h"
#include "GameObject.h"
#include "MovementComponent.h"
#include "Transform.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_RetreatAndWait)

bool BTTask_RetreatAndWait::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_RetreatAndWait";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Retreat Anim State", "retreat_anim_state", _retreatAnimState);
    PROPERTY_FLOAT_JSON("Retreat Duration", "retreat_duration", _retreatDuration, 0.05f, 5.f);
    PROPERTY_BOOL_JSON("Lock Facing To Target", "lock_facing_to_target", _lockFacingToTarget);
    PROPERTY_BOOL_JSON("Face Target", "face_target", _faceTarget);
    PROPERTY_BOOL_JSON("Use Sprint", "use_sprint", _useSprint);
    PROPERTY_BOOL_JSON("Request Anim End", "request_anim_end", _requestAnimEnd);
    PROPERTY_BOOL_JSON("Stop Movement On Finish", "stop_movement_on_finish", _stopMovementOnFinish);

    return true;
}

BTTask_RetreatAndWait::BTTask_RetreatAndWait()
{
}

BTTask_RetreatAndWait::BTTask_RetreatAndWait(const BTTask_RetreatAndWait& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _retreatAnimState(rhs._retreatAnimState)
    , _retreatDuration(rhs._retreatDuration)
    , _lockFacingToTarget(rhs._lockFacingToTarget)
    , _faceTarget(rhs._faceTarget)
    , _useSprint(rhs._useSprint)
    , _requestAnimEnd(rhs._requestAnimEnd)
    , _stopMovementOnFinish(rhs._stopMovementOnFinish)
    , _startedRetreat(false)
    , _elapsedRetreatTime(0.f)
    , _previousOrientRotationToMovement(false)
    , _selectedRetreatSide(rhs._selectedRetreatSide)
{
}

void BTTask_RetreatAndWait::Initialize()
{
    BTTask::Initialize();

    _startedRetreat = false;
    _elapsedRetreatTime = 0.f;
    _previousOrientRotationToMovement = false;

}

EBTNodeResult BTTask_RetreatAndWait::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto ownerTransform = owner->Get_Component<Transform>();
    auto movement = owner->Get_Component<MovementComponent>();
    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);

    if (!ownerTransform || !movement || !targetObject || targetObject->Is_Destroy())
    {
        Finish_Retreat(blackboard, owner);
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto targetTransform = targetObject->Get_Component<Transform>();
    if (!targetTransform)
    {
        Finish_Retreat(blackboard, owner);
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 toTarget = targetPos - ownerPos;
    toTarget.y = 0.f;

    if (!_startedRetreat)
    {
        _startedRetreat = true;
        _elapsedRetreatTime = 0.f;
        _previousOrientRotationToMovement = movement->Get_OrientRotationToMovement();

        _selectedRetreatSide = (Utils::RandomRange(0.f, 2.f) < 1.f) ? -1 : 1;

        if (_lockFacingToTarget)
            movement->Set_OrientRotationToMovement(false);

        if (_faceTarget)
        {
            Vec3 faceDirection = Utils::Safe_Normalize(toTarget, ownerTransform->Get_WorldForward());
            ownerTransform->LookAt(ownerPos + faceDirection);
        }

        if (!_retreatAnimState.empty())
            blackboard->Set_ValueAsString("AnimState", _retreatAnimState);

        if (_requestAnimEnd)
            blackboard->Set_ValueAsBool("AnimRequestEnd", true);
    }

    Vec3 backwardDirection = -ownerTransform->Get_WorldForward();
    backwardDirection.y = 0.f;
    backwardDirection = Utils::Safe_Normalize(backwardDirection, Vec3::Backward);

    Vec3 sideDirection = ownerTransform->Get_WorldRight();
    sideDirection.y = 0.f;
    sideDirection = Utils::Safe_Normalize(sideDirection, Vec3::Right);

    Vec3 retreatDirection = backwardDirection +
        sideDirection * (0.35f * static_cast<float>(_selectedRetreatSide));
    retreatDirection.y = 0.f;
    retreatDirection = Utils::Safe_Normalize(retreatDirection, backwardDirection);

    blackboard->Set_ValueAsFloat("MoveAxisX", retreatDirection.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", retreatDirection.z);
    blackboard->Set_ValueAsBool("Sprint", _useSprint);

    _elapsedRetreatTime += timeDelta;

    if (_elapsedRetreatTime < _retreatDuration)
    {
        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    Finish_Retreat(blackboard, owner);
    _lastResult = EBTNodeResult::Succeeded;
    return _lastResult;
}

void BTTask_RetreatAndWait::Finish_Retreat(const Shared<Blackboard>& blackboard, const Shared<GameObject>& owner)
{
    if (blackboard && _stopMovementOnFinish)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
    }

    if (owner)
    {
        auto movement = owner->Get_Component<MovementComponent>();
        if (movement && _lockFacingToTarget)
            movement->Set_OrientRotationToMovement(_previousOrientRotationToMovement);
    }

    _startedRetreat = false;
    _elapsedRetreatTime = 0.f;
}

Shared<BTTask_RetreatAndWait> BTTask_RetreatAndWait::Create()
{
    return make_shared<BTTask_RetreatAndWait>();
}

Shared<BTNode> BTTask_RetreatAndWait::Clone()
{
    auto clone = make_shared<BTTask_RetreatAndWait>(*this);
    clone->Initialize();

    return clone;
}
