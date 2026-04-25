#include "pch.h"
#include "BTTask_StrafeAroundTarget.h"

#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "Utils.h"

IMPLEMENT_REFLECTION(BTTask_StrafeAroundTarget)

bool BTTask_StrafeAroundTarget::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_StrafeAroundTarget";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Strafe Anim State", "strafe_anim_state", _strafeAnimState);
    PROPERTY_FLOAT_JSON("Strafe Duration", "strafe_duration", _strafeDuration, 0.05f, 5.f);
    PROPERTY_FLOAT_JSON("Ideal Distance", "ideal_distance", _idealDistance, 0.1f, 20.f);
    PROPERTY_FLOAT_JSON("Distance Correction Strength", "distance_correction_strength", _distanceCorrectionStrength, 0.f, 1.f);
    PROPERTY_BOOL_JSON("Lock Facing To Target", "lock_facing_to_target", _lockFacingToTarget);
    PROPERTY_BOOL_JSON("Use Sprint", "use_sprint", _useSprint);
    PROPERTY_BOOL_JSON("Stop Movement On Finish", "stop_movement_on_finish", _stopMovementOnFinish);

    return true;
}

BTTask_StrafeAroundTarget::BTTask_StrafeAroundTarget()
{
}

BTTask_StrafeAroundTarget::BTTask_StrafeAroundTarget(const BTTask_StrafeAroundTarget& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _strafeAnimState(rhs._strafeAnimState)
    , _strafeDuration(rhs._strafeDuration)
    , _idealDistance(rhs._idealDistance)
    , _distanceCorrectionStrength(rhs._distanceCorrectionStrength)
    , _lockFacingToTarget(rhs._lockFacingToTarget)
    , _useSprint(rhs._useSprint)
    , _stopMovementOnFinish(rhs._stopMovementOnFinish)
    , _startedStrafe(false)
    , _elapsedStrafeTime(0.f)
    , _selectedSide(rhs._selectedSide)
{
}

void BTTask_StrafeAroundTarget::Initialize()
{
    BTTask::Initialize();

    _startedStrafe = false;
    _elapsedStrafeTime = 0.f;
    _selectedSide = 1;
}

EBTNodeResult BTTask_StrafeAroundTarget::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    auto ownerTransform = owner->Get_Component<Transform>();
    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);

    if (!ownerTransform || !targetObject || targetObject->Is_Destroy())
    {
        Finish_Strafe(blackboard);
        return _lastResult = EBTNodeResult::Failed;
    }

    auto targetTransform = targetObject->Get_Component<Transform>();
    if (!targetTransform)
    {
        Finish_Strafe(blackboard);
        return _lastResult = EBTNodeResult::Failed;
    }

    if (!_startedStrafe)
    {
        _startedStrafe = true;
        _elapsedStrafeTime = 0.f;
        _selectedSide = (Utils::RandomRange(0.f, 2.f) < 1.f) ? -1 : 1;
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 toTarget = targetPos - ownerPos;
    toTarget.y = 0.f;

    const float distance = toTarget.Length();
    toTarget = Utils::Safe_Normalize(toTarget, ownerTransform->Get_WorldForward());

    Vec3 sideDir = Vec3::Up.Cross(toTarget);
    sideDir.y = 0.f;
    sideDir = Utils::Safe_Normalize(sideDir, ownerTransform->Get_WorldRight());
    sideDir *= static_cast<float>(_selectedSide);

    const float distanceError = distance - _idealDistance;
    Vec3 correctionDir = Vec3::Zero;

    if (fabsf(distanceError) > 0.1f)
        correctionDir = toTarget * (distanceError > 0.f ? 1.f : -1.f) * _distanceCorrectionStrength;

    Vec3 moveDir = sideDir + correctionDir;
    moveDir.y = 0.f;
    moveDir = Utils::Safe_Normalize(moveDir, sideDir);

    if (_lockFacingToTarget)
        ownerTransform->LookAt(ownerPos + toTarget);

    blackboard->Set_ValueAsFloat("MoveAxisX", moveDir.x);
    blackboard->Set_ValueAsFloat("MoveAxisY", moveDir.z);
    blackboard->Set_ValueAsBool("Sprint", _useSprint);

    if (!_strafeAnimState.empty())
        blackboard->Set_ValueAsString("AnimState", _strafeAnimState);

    _elapsedStrafeTime += timeDelta;

    if (_elapsedStrafeTime < _strafeDuration)
        return _lastResult = EBTNodeResult::InProgress;

    Finish_Strafe(blackboard);
    return _lastResult = EBTNodeResult::Succeeded;
}

void BTTask_StrafeAroundTarget::Finish_Strafe(const Shared<Blackboard>& blackboard)
{
    if (blackboard && _stopMovementOnFinish)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
    }

    _startedStrafe = false;
    _elapsedStrafeTime = 0.f;
}

Shared<BTTask_StrafeAroundTarget> BTTask_StrafeAroundTarget::Create()
{
    return make_shared<BTTask_StrafeAroundTarget>();
}

Shared<BTNode> BTTask_StrafeAroundTarget::Clone()
{
    auto clone = make_shared<BTTask_StrafeAroundTarget>(*this);
    clone->Initialize();

    return clone;
}
