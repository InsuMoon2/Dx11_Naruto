#include "pch.h"
#include "BTTask_JumpToTarget.h"

#include "Blackboard.h"
#include "GameObject.h"
#include "Transform.h"
#include "MovementComponent.h"

IMPLEMENT_REFLECTION(BTTask_JumpToTarget)

bool BTTask_JumpToTarget::Register_Properties()
{
    auto& info = GetStaticReflectionInfo();
    info.className = "BTTask_JumpToTarget";

    PROPERTY_STRING_JSON("Target Object Key", "target_object_key", _targetObjectKey);
    PROPERTY_STRING_JSON("Desired Combat Mode Key", "desired_combat_mode_key", _desiredCombatModeKey);
    PROPERTY_STRING_JSON("Jump Start Anim State", "jump_start_anim_state", _jumpStartAnimState);
    PROPERTY_STRING_JSON("Air Approach Anim State", "air_approach_anim_state", _airApproachAnimState);
    PROPERTY_FLOAT_JSON("Acceptance Radius", "acceptance_radius", _acceptanceRadius, 0.1f, 20.f);
    PROPERTY_FLOAT_JSON("Horizontal Speed", "horizontal_speed", _horizontalSpeed, 0.1f, 50.f);
    PROPERTY_FLOAT_JSON("Vertical Approach Speed", "vertical_approach_speed", _verticalApproachSpeed, 0.1f, 50.f);
    PROPERTY_FLOAT_JSON("Startup Duration", "startup_duration", _startupDuration, 0.01f, 1.0f);
    PROPERTY_BOOL_JSON("Disable Gravity During Approach", "disable_gravity_during_approach", _disableGravityDuringApproach);

    PROPERTY_FLOAT_JSON("Target Lead Time", "target_lead_time", _targetLeadTime, 0.f, 0.5f);
    PROPERTY_FLOAT_JSON("Homing Interval", "homing_interval", _homingInterval, 0.01f, 0.5f);
    PROPERTY_FLOAT_JSON("Hard Lock After", "hard_lock_after", _hardLockAfter, 0.f, 1.f);
    PROPERTY_FLOAT_JSON("Vertical Lerp Speed", "vertical_lerp_speed", _verticalLerpSpeed, 0.1f, 30.f);
    PROPERTY_FLOAT_JSON("Max Approach Duration", "max_approach_duration", _maxApproachDuration, 0.1f, 2.f);

    return true;
}

BTTask_JumpToTarget::BTTask_JumpToTarget()
{
}

BTTask_JumpToTarget::BTTask_JumpToTarget(const BTTask_JumpToTarget& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _desiredCombatModeKey(rhs._desiredCombatModeKey)
    , _jumpStartAnimState(rhs._jumpStartAnimState)
    , _airApproachAnimState(rhs._airApproachAnimState)
    , _acceptanceRadius(rhs._acceptanceRadius)
    , _horizontalSpeed(rhs._horizontalSpeed)
    , _verticalApproachSpeed(rhs._verticalApproachSpeed)
    , _startupDuration(rhs._startupDuration)
    , _disableGravityDuringApproach(rhs._disableGravityDuringApproach)
{
}

void BTTask_JumpToTarget::Initialize()
{
    BTTask::Initialize();

    _startedJump = false;
    _switchedToApproach = false;
    _elapsedJumpTime = 0.f;
    _savedGravityEnabled = true;
    _hasSavedGravityState = false;

    _dashTargetPosition = Vec3::Zero;
    _homingElapsed = 0.f;
}

void BTTask_JumpToTarget::Reset_AirApproachState(const Shared<MovementComponent>& movement)
{
    if (movement && _hasSavedGravityState)
        movement->Set_GravityEnabled(_savedGravityEnabled);

    _startedJump = false;
    _switchedToApproach = false;
    _elapsedJumpTime = 0.f;
    _savedGravityEnabled = true;
    _hasSavedGravityState = false;
}

EBTNodeResult BTTask_JumpToTarget::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
        return _lastResult = EBTNodeResult::Failed;

    const int32 desiredCombatMode = blackboard->HasKey(_desiredCombatModeKey)
        ? blackboard->Get_ValueAsInt(_desiredCombatModeKey)
        : 0;

    if (desiredCombatMode == 0)
    {
        auto movement = owner->Get_Component<MovementComponent>();
        Reset_AirApproachState(movement);
        return _lastResult = EBTNodeResult::Failed;
    }

    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!targetObject || targetObject->Is_Destroy())
    {
        auto movement = owner->Get_Component<MovementComponent>();
        Reset_AirApproachState(movement);
        return _lastResult = EBTNodeResult::Failed;
    }

    auto ownerTransform = owner->Get_Component<Transform>();
    auto ownerMovement = owner->Get_Component<MovementComponent>();
    auto targetTransform = targetObject->Get_Component<Transform>();
    auto targetMovement = targetObject->Get_Component<MovementComponent>();

    if (!ownerTransform || !ownerMovement || !targetTransform)
    {
        Reset_AirApproachState(ownerMovement);
        return _lastResult = EBTNodeResult::Failed;
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 targetVelocity = Vec3::Zero;
    if (targetMovement)
        targetVelocity = targetMovement->Get_Velocity();

    blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
    blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
    blackboard->Set_ValueAsBool("Sprint", false);

    if (!_startedJump)
    {
        if (!_jumpStartAnimState.empty())
            blackboard->Set_ValueAsString("AnimState", _jumpStartAnimState);

        ownerMovement->Start_Jump();

        if (_disableGravityDuringApproach && !_hasSavedGravityState)
        {
            _savedGravityEnabled = ownerMovement->Is_GravityEnabled();
            _hasSavedGravityState = true;
            ownerMovement->Set_GravityEnabled(false);
        }

        _startedJump = true;
        _switchedToApproach = false;
        _elapsedJumpTime = 0.f;
        _homingElapsed = 0.f;
        _dashTargetPosition = Vec3::Zero;

        return _lastResult = EBTNodeResult::InProgress;
    }

    _elapsedJumpTime += timeDelta;

    if (!_switchedToApproach &&
        (_elapsedJumpTime >= _startupDuration || !ownerMovement->Is_OnGround()))
    {
        if (!_airApproachAnimState.empty())
            blackboard->Set_ValueAsString("AnimState", _airApproachAnimState);

        _dashTargetPosition = targetPos + targetVelocity * _targetLeadTime;
        _dashTargetPosition.y = targetPos.y + 0.6f;

        Vec3 velocity = ownerMovement->Get_Velocity();
        velocity.y = 0.f;
        ownerMovement->Set_Velocity(velocity);

        _switchedToApproach = true;
        _homingElapsed = 0.f;
    }

    if (_switchedToApproach)
    {
        _homingElapsed += timeDelta;

        if (_elapsedJumpTime < _hardLockAfter && _homingElapsed >= _homingInterval)
        {
            Vec3 nextDashTarget = targetPos + targetVelocity * _targetLeadTime;
            nextDashTarget.y = targetPos.y + 0.6f;

            _dashTargetPosition = Vec3::Lerp(_dashTargetPosition, nextDashTarget, 0.35f);
            _homingElapsed = 0.f;
        }

        Vec3 toDashTarget = _dashTargetPosition - ownerPos;
        const float distance = toDashTarget.Length();

        Vec3 horizontalDir = toDashTarget;
        horizontalDir.y = 0.f;
        horizontalDir = Utils::Safe_Normalize(horizontalDir, ownerTransform->Get_WorldForward());

        ownerTransform->LookAt(ownerPos + horizontalDir);

        Vec3 velocity = ownerMovement->Get_Velocity();
        velocity.x = horizontalDir.x * _horizontalSpeed;
        velocity.z = horizontalDir.z * _horizontalSpeed;

        const float desiredY = _dashTargetPosition.y;
        const float nextYVelocity = (desiredY - ownerPos.y) * _verticalLerpSpeed;

        velocity.y = ::clamp(
            nextYVelocity,
            -_verticalApproachSpeed,
            _verticalApproachSpeed);

        ownerMovement->Set_Velocity(velocity);

        if (distance <= _acceptanceRadius || _elapsedJumpTime >= _maxApproachDuration)
        {
            Reset_AirApproachState(ownerMovement);
            return _lastResult = EBTNodeResult::Succeeded;
        }
    }

    return _lastResult = EBTNodeResult::InProgress;
}


Shared<BTTask_JumpToTarget> BTTask_JumpToTarget::Create()
{
    return make_shared<BTTask_JumpToTarget>();
}

Shared<BTNode> BTTask_JumpToTarget::Clone()
{
    auto clone = make_shared<BTTask_JumpToTarget>(*this);
    clone->Initialize();
    return clone;
}
