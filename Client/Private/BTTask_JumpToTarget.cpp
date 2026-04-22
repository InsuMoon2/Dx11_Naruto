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
    PROPERTY_STRING_JSON("Jump Anim State", "jump_anim_state", _jumpAnimState);

    PROPERTY_FLOAT_JSON("Acceptance Radius", "acceptance_radius", _acceptanceRadius, 0.1f, 20.f);
    PROPERTY_FLOAT_JSON("Horizontal Speed", "horizontal_speed", _horizontalSpeed, 0.1f, 50.f);

    return true;
}

BTTask_JumpToTarget::BTTask_JumpToTarget()
{
}

BTTask_JumpToTarget::BTTask_JumpToTarget(const BTTask_JumpToTarget& rhs)
    : BTTask(rhs)
    , _targetObjectKey(rhs._targetObjectKey)
    , _desiredCombatModeKey(rhs._desiredCombatModeKey)
    , _jumpAnimState(rhs._jumpAnimState)
    , _acceptanceRadius(rhs._acceptanceRadius)
    , _horizontalSpeed(rhs._horizontalSpeed)
    , _startedJump(false)
{
}

void BTTask_JumpToTarget::Initialize()
{
    BTTask::Initialize();

    _startedJump = false;
}

EBTNodeResult BTTask_JumpToTarget::Update(float timeDelta)
{
    auto blackboard = _blackboard.lock();
    auto owner = _owner.lock();

    if (!blackboard || !owner)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    // 공중 공격 모드가 아니면 이 태스크는 탈 이유가 없으므로 실패를 반환해 다른 브랜치로 넘긴다.
    const int32 desiredCombatMode = blackboard->HasKey(_desiredCombatModeKey)
        ? blackboard->Get_ValueAsInt(_desiredCombatModeKey)
        : 0;

    if (desiredCombatMode == 0)
    {
        _startedJump = false;
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto targetObject = blackboard->Get_ValueAsObject(_targetObjectKey);
    if (!targetObject || targetObject->Is_Destroy())
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    auto ownerTransform = owner->Get_Component<Transform>();
    auto ownerMovement = owner->Get_Component<MovementComponent>();
    auto targetTransform = targetObject->Get_Component<Transform>();

    if (!ownerTransform || !ownerMovement || !targetTransform)
    {
        _lastResult = EBTNodeResult::Failed;
        return _lastResult;
    }

    const Vec3 ownerPos = ownerTransform->Get_WorldPosition();
    const Vec3 targetPos = targetTransform->Get_WorldPosition();

    Vec3 horizontalDir = targetPos - ownerPos;
    horizontalDir.y = 0.f;

    const float horizontalDistance = horizontalDir.Length();
    horizontalDir = Utils::Safe_Normalize(horizontalDir, ownerTransform->Get_WorldForward());

    // 점프 접근 중에는 계속 타겟 방향을 바라보게
    ownerTransform->LookAt(ownerPos + horizontalDir);

    if (!_startedJump)
    {
        blackboard->Set_ValueAsFloat("MoveAxisX", 0.f);
        blackboard->Set_ValueAsFloat("MoveAxisY", 0.f);
        blackboard->Set_ValueAsBool("Sprint", false);
        blackboard->Set_ValueAsString("AnimState", _jumpAnimState);

        ownerMovement->Start_Jump();

        Vec3 velocity = ownerMovement->Get_Velocity();
        velocity.x = horizontalDir.x * _horizontalSpeed;
        velocity.z = horizontalDir.z * _horizontalSpeed;
        ownerMovement->Set_Velocity(velocity);

        _startedJump = true;

        _lastResult = EBTNodeResult::InProgress;
        return _lastResult;
    }

    Vec3 velocity = ownerMovement->Get_Velocity();
    velocity.x = horizontalDir.x * _horizontalSpeed;
    velocity.z = horizontalDir.z * _horizontalSpeed;
    ownerMovement->Set_Velocity(velocity);

    if (horizontalDistance <= _acceptanceRadius)
    {
        _startedJump = false;
        _lastResult = EBTNodeResult::Succeeded;
        return _lastResult;
    }

    _lastResult = EBTNodeResult::InProgress;
    return _lastResult;
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
